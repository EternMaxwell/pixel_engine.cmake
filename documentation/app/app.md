# App

App in pixel_engine.cmake is the core interface where you can interact with the pixel_engine.cmake engine.

A little example of how you can use app in pixel_engine.cmake:

```cpp
#include <pixel_engine/app.h>
#include <iostream>

using namespace pixel_engine::prelude;

void hello_epxg() {
    std::cout << "Hello Pixel Engine Cmake!" << std::endl;
}

int main() {
    App app;
    app.add_system(hello_epxg)
        .in_stage(LoopStage::Update)
    ->run();
}
```

This will print out the following:

```cpp
// some info from app
Hello Pixel Engine Cmake!
// some info from app
```

Also notice that we specified the stage in which the system should be run.

## Stages

Stages in app is used to control the run sequence of the systems. `App` has 4 basic stages, `startup`, `transition`, `loop`, `exit`.

Systems in `startup` will be run at the beginning of the app.run(), and will only be run once.
Systems in `transition` and `loop` will be run after StartStage and will run once or loop until recieved AppExit Event.

Systems in `exit` will run at the end of app.run() and will only be run once.

The sequence of the four basic stages are represented below:

```cpp
/**
    -> startup -> loop -> transition -> exit
                <-     loop here    <-
 */
```

Stages with Substages can be attached to a BasicStage with src and dst SubApp specified.
Stage and its SubStages is represented with a enum type, and can be added to a custom app inheriting the App, in constructor like following:

```cpp
enum CustomStage {
    First,
    Second,
};

struct CustomApp : App {
    CustomApp() : App() {
        runner.assign_loop_stage<MainSubApp, MainSubApp>(
            First, Second
        );
        // the sequence of the stages is the sequence of them in the argument.
    }
}
```

*Note that the two template type is the type of the `src` subapp (the first) and the `dst` subapp (the second). There also a lot interfaces relating to assign stages but we will talk about that later.*

However, most of the time, you are just using the App the engine provides you, which has the following stages:

```cpp
enum StartStage { PreStartup, Startup, PostStartup };
// in startup, with src = dst = MainSubApp
enum StateTransitionStage { StateTransit };
// in transition, with src = dst = MainSubApp
enum LoopStage { First, PreUpdate, Update, PostUpdate, Last };
// in loop, with src = dst = MainSubApp
enum RenderStage { Prepare, PreRender, Render, PostRender };
// in loop, with src = dst = MainSubApp, and after LoopStage
enum ExitStage { PreShutdown, Shutdown, PostShutdown };
// in exit, with src = dst = MainSubApp
```

To specify the stage to use for a system, call `in_stage(stage)` on the return value of `add_system`, like in the first example.

## Ecs

### entity components

`pixel_engine.cmake` is a ecs based light weight game engine. In oop, objects may be represented like this:

```cpp
class Creature {
    virtual void take_damage(float dmg);
}

class Enemy : public Creature {
    float health = 100.0f;
    void take_damage(float dmg) {
        health -= dmg;
        if (health <= 0.0f)
            despawn();
    }
}

class Player : public Creature {
    float health = 100.0f;
    void take_damage(float dmg) {
        health -= dmg;
        if (health <= 0)
            game_over();
    }
}
```

Then when a bullet hits a Creature, it calls `take_damage` and invoke the class' specific `take_damage` function. This is very reader-friendly, and pretty straight forward, and has ability to extend to more features. However it is not cpu friendly, since in cpp, objects like this have to be stored using pointers and allocated in the heap randomly every where. Since the cpu need to load data from memory to cache, these unpacked data may lead to constantly transforming data between cache and RAM which lead to bad performance.

However in ecs, objects is seperated to entity and components, where entity is the handle and components is the actual data. The same type of components are packed in one storage. Since its packed, its very cache friendly, can have much more improvement on performance. To use the entity component part of the engine, you can use `Command` and `Query`, the first one for spawn and despawn, second one for retrieving data.
For example:

```cpp
struct ExampleComponent {
    int data;
};

struct ExampleTag {};

// create
void create_object(Command cmd) {
    cmd.spawn(ExampleComponent{}, ExampleTag{});
}

// query
void print_data(Query<Get<ExampleComponent>, With<ExampleTag>> query) {
    for (auto [component] : query.iter()) {
        // in this case auto or auto&& is the same since the data retrieved
        // from Query is left reference of the actual data.
        std::cout << "component data: " << component.data << std::endl;
    }
}

// destroy
void destroy_object(
    Command cmd,
    Query<Get<Entity>, With<ExampleComponent, ExampleTag>> query
) {
    for (auto [id] : query.iter()) {
        cmd.entity(id).despawn();
    }
}
```

Like above, in ecs, the entity and components does not necessarily have member functions to modify and get the data, or construct or destruct them. These is all done by systems, in the example above, it is functions with proper arguments. The ecs library used in pixel_engine.cmake is [EnTT](https://github.com/skypjack/entt).

### Systems

`System` in pixel_engine.cmake can also be seen as a task in a Task Graph. Systems in app are run parrallel. The synchronization is handled by`app` automatically, based on there arguments. So there is no need to concern about the data confliction most of the time. The allowed arguments are `Command`, `Query`, `Extract`, `Resource`, `EventReader`, `EventWriter`.

And each argument refers to a subapp declared in the stage the system is in, when assigning the stage. `Command`, `Query`, `EventWriter` bounds to the dst subapp, and modify data in the dst subapp. `Extract`, `Resource`, `EventReader` bounds to src subapp, and can modify `Resource` and `EventReader` but not `Extract`.

#### Query

`Query` is used to get the data of components of the entity satisfies the `Query`. A complete `Query` is `Query<Get<Gets...>, With<Withs...>, Without<Withouts...>>`. This queries for the entities that have `Gets...` and `Withs...` and does not have `Withouts...`, and retrieves datas in `Gets...`.

The type of `Gets` should not be reference but can be `const`. And is recommanded to use `const` if you are not going to modify the data or whenever possible, since this is used in the runner to determine whether or not synchronize the system with others.

Add `Entity` or `entt::entity` at the first in Gets if you also want to get the id of the entity.
You can either iterate on all the entity in the query or get a single one. The API for `Query`:

```cpp
auto iter();
// returns an iterator which has the value type:
// std::tuple<Gets&...> or std::tuple<entt::entity, Gets&...> based on whether or not queries entity.
// this may not contain the entity spawned after the Query is created, which is mostly in the same
// system.

std::tuple<Gets&...> get(entt::entity entity);
std::tuple<entt::entity, Gets&...> get(entt::entity entity);
// These two functions are used to get the components of certain entity. This should return
// successfully even if it is a new entity spawned in the same system

bool contains(entt::entity entity);
// whether or not the Query contains the given entity. Newly created entity will return true.
// At least in the test.

auto single();
// get a single entity in the view, does not guarantee certain one to return, it is up to you
// to get the one you want using proper Withs and Withouts.
// the return type of single is std::optional<decltype(get(entt::entity{}))>.

void for_each(std::function<void(Gets&...));
void for_each(std::function<void(entt::entity, Gets&...));
// these two will iterate on all entities in the query, and is faster than iter().
```

A system with `Query`:

```cpp
void func(Query<Get<Entity, Health, Defense>, With<CanDamage>, Without<Immortal> query) {
    if (!query.single().has_value())
        return;
    auto [id, health, defense] = query.single().value();
    // do something

    // or
    for (auto [id, health, defense] : query.iter()) {
        // do something
    }

    // or
    query.for_each([](auto id, auto& health, auto& defense) {
        // do something.
    })
}
```

#### Extract

`Extract` is almost the same as `Query`, except that all the Gets in Extract are transformed to const type implicitly and `Extract` bounds to `src` subapp while `Query` bounds to `dst` subapp.
`Extract` is designed to be used for transmitting data between subapps.

#### Resource

`Resource` is not a part of ecs, but can be useful in games or any other apps. Unlike `Query` and `Extract`, a certain type of resource `Resource<T>` only has one instance in one subapp, `Resource`s can be created with `Command`, which will be talked about later.

To access the actual resource in `Resource<T>`, use `operator->()` to access the `T*`.

A system with `Resource`:

```cpp
void func(Resource<FileReader> file_reader) {
    if (!file_reader.has_value())
        return;
    auto data = file_reader->read("file-name");
    // or you can use operator* to get a left reference of the resource.
}
```

#### Command

`Command` is used to create objects, insert resource and get the `EntityCommand`. The API for `Command`:

```cpp
template <typename... Args>
entt::entity spawn(Args&&... args);
// Entity is an alias of entt::entity, which in 64bit platform it's uint64_t, uint32_t in 32bit.
// args is the components of the entity, or can be a bundle of components which will be unpacked to
// the components in the bundle and added to the entity.
// returns the entity id of the newly created entity.

template <typename T, typename... Args>
void insert_resource(Args&&... args);
// this calls the following constructor:
// T(Args(&)... args);
// do nothing if already exists

template <typename T>
void insert_resource(T&& t);
// this calls the copy or move constructor based on the type of T.
// do nothing if already exists

template <typename T>
void init_resource();
// this calls the default constructor or a constructor with no arguments.
// do nothing if already exists

template <typename T>
void remove_resource();
// remove the resource from the target subapp.
// do nothing if it does not exist

EntityCommand entity(entt::entity entity);
// this returns the EntityCommand for the specified entity;
```

#### EntityCommand

`EntityCommand` is used to spawn child entity for the bound entity or despawn the bound entity with or without its children despawned. The API for `EntityCommand`:

```cpp

template <typename... Args>
entt::entity spawn(Args&&... args);
// This spawn's an entity which will contain a component Parent when created,
// and will be added to the entity relation tree. The Parent only contains
// a id of type entt::entity, referencing the parent entity, which in this
// case is the entity bound with the EntityCommand.
// This function returns the spawned child entity id.

template <typename... Args>
void emplace(Args&&... args);
// This emplace new components to the entity.

template <typename... Args>
void erase();
// remove specified components in the bound entity.
// Args can not be Bundle.

void despawn();
// despawn the entity. Remove the entity in the subapp.

void despawn_recurse();
// despawn the entity and all its children.
```

*Note that the despawn is not immediate, and will be performed at the end of each basic stage.
But spawn is immediate.*

#### EventReader & EventWriter

`Event`s is not a part of ecs but useful in games or any other app. `EventWriter` is used to write event to the internal event queue. `EventReader` is used to read events from the internal queue.
A simple example:

```cpp
struct TestEvent {
    int data;
};

void write_event(EventWriter<TestEvent> event_writer) {
    // if some conditions are satisfied.
    event_writer.write(TestEvent{0});
}

void read_event(EventReader<TestEvent> event_reader) {
    for (auto& evt : event_reader.read()) {
        std::cout << "Read TestEvent, data is:" << evt.data << std::endl;
    }
}
```

API for `EventWriter` and `EventReader`:

```cpp
template <typename Evt>
auto& EventWriter<Evt>::write(Evt evt);
// Write event to the certain type event queue.
// Returns the EventWriter itself.

template <typename Evt>
EventReader<Evt>::event_iter EventReader<Evt>::read();
// Get a iterator of the events.

template <typename Evt>
bool EventReader<Evt>::empty();
// Check if the event queue is empty.

template <typename Evt>
void clear();
// Clear the event queue manually.
```

The events created are guaranteed to exist at least one frame. *Events will be removed at the end of the frame after the frame they are writen.*

These are all the infomation you need to know to write a system.

## Add systems to a app

To add systems to app, use `add_system`, which is a overloaded and template function in `app`.
A simple example has be writen at the first of this page. We will talk deeper here.

`add_system` accepts several different type of input arguments, but the preferred and recommanded one is `add_system(void(func*)(Args...))`. All `add_system` will return a `AddSystemReturn` object, which will reference a `SystemNode` ptr in `Runner` in `App`. `SystemNode` is the internal wrapper of a system, containing more detailed informations for the `Runner` to run, and the user has no need to understand it. All you need to know currently is that a `SystemNode` can be mapped using a function ptr, which is the function ptr in the `add_system` input arguments. `AddSystemReturn` has functions for the user or the caller to modify the detailed information in referenced `SystemNode`, like in which stage, in what set, has what conditions, the relationship with other systems and in what state should it be run.

The API for `AddSystemNode`:

```cpp
template <typename... Sets>
AddSystemReturn& AddSystemReturn::in_set(Sets... sets);
// Attach the target system node to the sets.
// Return the AddSystemReturn itself.

template <typename... Addrs>
AddSystemReturn& AddSystemReturn::before(Addrs... addrs);
// Addrs should be function pointers.
// This tells the Runner that this system should be run before systems mapped by addrs.
// And the systems mapped by addrs should run after the target system has finished.
// Do nothing if the mapped systems and the target system are not in the same substage.
// Return the AddSystemReturn itself.

template <typename... Addrs>
AddSystemReturn& AddSystemReturn::after(Addrs... addrs);
// Addrs should be function pointers.
// This tells the Runner that this system should be run after all systems mapped by addrs
// has finished.
// Return the AddSystemReturn itself.

template <typename T>
AddSystemReturn& AddSystemReturn::in_stage(T stage);
// Set the stage the target system is in.
// Return the AddSystemReturn itself.

AddSystemReturn& AddSystemReturn::use_worker(std::string name);
// Set the Worker the target system should run in.
// A Worker can be seen as a thread pool in the runners.

AddSystemReturn& AddSystemReturn::run_if(std::shared_ptr<BasicSystem<bool>> cond);
// Add conditions to the target system.
// The system will run only if all the conditions have passed. But it will still be seen
// finished even if conditions are not passed all.

template <typename T>
AddSystemReturn& AddSystemReturn::on_enter(const T& state);
// This system will be run if entering state T::state.
// This need the substage in transition basic stage, otherwise sending warning and ignore.

template <typename T>
AddSystemReturn& AddSystemReturn::on_exit(const T& state);
// This system will be run if exiting state T::state.
// This need the substage in transition basic stage, otherwise sending warning and ignore.

template <typename T>
AddSystemReturn& AddSystemReturn::on_change();
// This system will be run if state T is changing.
// This need the substage in transition basic stage, otherwise sending warning and ignore.
```

*Note that systems that are not yet registered can be used in after and before using their function pointers.*

There are some concepts we didn't talk yet: `SystemSet`, `Worker`, `Condition`.

### SystemSet

`SystemSet` can be seen as a set of systems, and is used to configure the dependencies of the systems in the set. It can be represented by a enum type, for example:

```cpp
enum ExampleSet {
    Set1,
    Set2,
    Set3,
};
```

To add the sets to the app:

```cpp
App app;
app.configure_sets(Set1, Set2, Set3);
```

This tells the `app` and its `runner` that systems in `Set1` are before `Set2`, systems in `Set2` are before `Set3`.

The arguments in `configure_sets` should be the same enum type. And different enum types of sets have no relationships.

### Worker

`Worker` is a thread pool in runner. `Runner` can contain more than one thread pool, for multiple usage. Systems using different worker still maintain their dependencies.

`Worker`s are useful if you want something to run on exactly one thread, or you want it to run on thread pool with wanted thread counts. The default workers are: `"default"` with 8 threads, `"single"` with 1 thread. The default worker used if not specified is `"default"`.

`Worker`s can be added to runner:

```cpp
void Runner::add_worker(std::string name, int num_threads);
// Arguments are the name of the new worker and the thread count of the worker.
```

### Conditions

`Conditions` are the conditions that need to be satisfied in order to run the target system.
Users can specify conditions using its constructor:

```cpp
std::shared_ptr<BasicSystem<bool>> cond
        = std::make_shared<Condition<Args...>>(bool(func*)(Args...));
// or
std::shared_ptr<BasicSystem<bool>> cond
        = std::make_shared<Condition<Args...>>(std::function<bool(Args...)>);
```

Or use the helper functions to create conditions easily, like `in_state`.

### System Tuple

`System` can also be added using tuple of functions. For example:

```cpp
App app;
app.add_system(std::make_tuple(func1, func2, func3)).in_stage(Update);
```

In this case, the 3 functions can be viewed as a single function, this is useful if you want the functions to be 'packed', which means func3 will run right after func2, which run right after func1, while they have different arguments. Internally, pointer `func1`, `func2`, `func3` all maps to the same `SystemNode`. So to specify the dependency of this system, either using `func1` or `func2` or `func3` can do the job.

## States

`State` is a special type of resource. `State`s can be added to `App` using:

```cpp
template <typename T>
App& App::insert_state(T state);

template <typename T>
App& App::init_state();
```

These two functions will add `Resource<State<T>>` and `Resource<NextState<T>>` to the resources to the main subapp in app. The first function gives them the specified initial value, the second one gives them the default value.

### Usage

The state is used in transition stage systems and conditions.

The previous `in_state` conditions passes if `Resource<State<T>>` is the same as the the specified state. To set the next state, use `void State<T>::set_state(const T& state)`.

Then this will be used in `on_enter`, `on_exit`, `on_change` systems.

- `on_enter(state)` systems will be run if `NextState` doesn't equal to `State` and `NextState` equals to `state`.
- `on_exit(state)` systems will be run if `NextState` doesn't equal to `State` and `State` equal to `state`.
- `on_change<T>()` systems will be run if `NextState` doesn't equal to `State`.
A simple example:

```cpp
enum RunStates {
    Pause,
    Running,
};

enum GameState {
    MainMenu,
    InGame,
};

void load_game(Args... args) {
    // loading game or save resources
}

void unload_game(Args... args) {
    // unloading and save game
}

void update(Args... args) {
    // update game
}

void pause(Args... args, Resource<NextState<RunStates>> run_state) {
    // check input and pause the game for example if ecs pressed.
}

void resume(Args... args, Resource<NextState<RunStates>> run_state) {
    // check input and resume game.
}

void enter_game(Args... args, Resource<NextState<GameState>> game_state) {
    // check input and entering game
}

void to_menu(Args... args, Resource<NextState<GameState>> game_state) {
    // check input and return to main menu
}

void create_main_menu(Args... args) {
    // create main menu components or enable them to be visible and interactable.
}

int main() {
    App app;
    app.insert_state(Running)
        .insert_state(MainMenu)
        .enable_loop()
        .add_system(load_game).in_stage(StateTransit).on_enter(InGame)
        ->add_system(unload_game).in_stage(StateTransit).on_exit(InGame)
        ->add_system(pause).in_stage(Update).run_if(in_state(Running))
        ->add_system(resume).in_stage(Update).run_if(in_state(Pause))
        ->add_system(update).in_stage(Update).run_if(in_state(Running))
        ->add_system(enter_game).in_stage(Update).run_if(in_state(MainMenu))
        ->add_system(to_menu).in_stage(Update).run_if(in_state(InGame))
        ->add_system(create_main_menu).in_stage(StateTransit).on_enter(MianMenu)
        ->run();
}
```

## Plugin

`Plugin` is used to setup a app easily with presets. `Plugin` is a virtual struct, with only one function: `void build(App& app)`. You are able to access all functions in `App` in `Plugin`. When adding `Plugin`s to app, their contents are copied to an internal plugin container and later copied to all subapp as a resource: `Resource<PluginType>`. So you are able to add custom data in your plugin and access the data in functions.

An simple example:

```cpp
struct TestPlugin;

void func(Resource<TestPlugin> test_plugin);

struct TestPlugin : Plugin {
    int data;
    void build(App& app) override {
        app.add_system(func).in_stage(Update);
    }
}

void func(Resource<TestPlugin> test_plugin) {
    std::cout << "data in test plugin: " << test_plugin->data << std::endl;
}

int main() {
    App app;
    app.add_plugin(TestPlugin{})->run();
}
```

You can also get the plugin in an app.

```cpp
std::shared_ptr<TestPlugin> = app.get_plugin<TestPlugin>();
```

This returns the internal plugin. Since they will be copied to subapps later in the building process of `App`, changing the data here is accepted.

A simple example:

```cpp
struct Plugin1;
struct Plugin2;

void func1(Resource<Plugin1>);

struct Plugin1 : Plugin {
    int data;
    void build(App& app) override {
        app.add_system(func1).in_stage(Update);
    }
};

struct Plugin2 : Plugin {
    void build(App& app) override {
        app.get_plugin<Plugin1>()->data = 100;
    }
};

void func1(Resource<Plugin1> p) {
    std::cout << "Data: " << p->data << std::endl;
}

int main() {
    App app;
    app.add_plugin(Plugin2{})
        ->add_plugin(Plugin1{})
        ->run();
}
```

This will print out:

```cpp
Data: 100
```

*Note that in the example, we add plugin2 before plugin1, but the data in plugin1 is still changed, and there's no error in plugin2 when accessing  plugin1. This is because that when plugins are added, they are not built directly, but added to the internal container, then the app will call their build function in building process in the begining of  `run()`.*

### Combining plugins

You can also combining plugins by doing the following:

```cpp
struct Plugin1 : Plugin {
    void build(App& app) override {
        // do the building
    }
};

struct Plugin2 : Plugin {
    void build(App& app) override {
        // do the building
    }
};

struct CombinedPlugin : Plugin {
    void build(App& app) override {
        app.add_plugin(Plugin1{})
            ->add_plugin(Plugin2{});
    }
};
```

## Beta features

These are beta features that still being developed and not yet well tested.

### removing systems

Calling `Runner::remove_system(void* function_ptr)` will remove the mapped system.

### removing plugins

Calling `App::remove_plugin<PluginType>()` will remove all the systems added by plugin in loop or transition basic stage.
