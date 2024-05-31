#include <pixel_engine/entity/entity.h>

#include <iostream>
#include <random>

using namespace pixel_engine;

struct Health {
    float life;
};

struct Position {
    float x, y;
};

struct WithChild {};

void start(entity::Command command) {
    std::cout << "start" << std::endl;
    for (int i = 0; i < 50; i++) {
        using namespace std;
        int min = 0, max = 100;
        random_device seed;
        ranlux48 engine(seed());
        uniform_int_distribution<> distrib(min, max);
        int random = distrib(engine);
        if (random > 50) {
            command.spawn(Health{.life = 100.0f}, Position{.x = 0, .y = 0});
        } else {
            command.spawn(Health{.life = 100.0f}, WithChild{});
        }
    }
    std::cout << std::endl;
}

void create_child(
    entity::Command command,
    entity::Query<std::tuple<entt::entity, WithChild>, std::tuple<>> query) {
    std::cout << "create_child" << std::endl;
    for (auto [entity] : query.iter()) {
        std::cout << "entity with child: " << static_cast<int>(entity)
                  << std::endl;
        command.entity(entity).spawn(Health{.life = 50},
                                     Position{.x = 0, .y = 0});
    }
    std::cout << std::endl;
}

void despawn(
    entity::Command command,
    entity::Query<std::tuple<entt::entity, WithChild>, std::tuple<>> query) {
    std::cout << "despawn" << std::endl;
    for (auto [entity] : query.iter()) {
        command.entity(entity).despawn_recurse();
    }
    std::cout << std::endl;
}

void add_res(entity::Command command) {
    std::cout << "add_res" << std::endl;
    command.insert_resource(Health{.life = 100.0f});
    std::cout << std::endl;
}

void print(entity::Query<std::tuple<entt::entity, const Health, const Position>,
                         std::tuple<>>
               query) {
    std::cout << "print" << std::endl;
    auto [entity, health, pos] = query.single().value();
    std::cout << "single entity: " << static_cast<int>(entity) << ": "
              << health.life << std::endl;

    for (auto [entity, health, pos] : query.iter()) {
        std::cout << static_cast<int>(entity) << ": " << health.life
                  << std::endl;
    }
    std::cout << std::endl;
}

void access_no_entity_health(
    entity::Query<std::tuple<const Health>, std::tuple<>> query) {
    std::cout << "access_no_entity_health" << std::endl;
    for (auto [health] : query.iter()) {
        std::cout << health.life << std::endl;
    }
    auto single = query.single();
    std::cout << "single: ";
    if (single.has_value()) {
        auto [health] = single.value();
        std::cout << health.life << std::endl;
    }
    std::cout << std::endl;
}

void print_res(entity::Resource<const Health> res) {
    std::cout << "print_res" << std::endl;
    if (res.has_value()) {
        std::cout << "resource: " << res.value().life << std::endl;
    }
    std::cout << std::endl;
}

void access_not_exist_res(entity::Resource<Position> res) {
    std::cout << "access_not_exist_res" << std::endl;
    if (res.has_value()) {
        std::cout << "resource: " << res.value().x << std::endl;
    } else {
        std::cout << "resource not exist" << std::endl;
    }
    std::cout << std::endl;
}

void add_not_exist_res(entity::Command command) {
    std::cout << "add_not_exist_res" << std::endl;
    command.init_resource<Position>();
    std::cout << std::endl;
}

void remove_not_exist_res(entity::Command command) {
    std::cout << "remove_not_exist_res" << std::endl;
    command.remove_resource<Position>();
    std::cout << std::endl;
}

void change_res(entity::Resource<Health> res) {
    std::cout << "change_res" << std::endl;
    if (res.has_value()) {
        res.value().life = 50.0f;
    }
    std::cout << std::endl;
}

void write_event(entity::EventWriter<Health> writer) {
    std::cout << "write_event" << std::endl;
    writer.write(Health{.life = 100.0f});
    writer.write(Health{.life = 50.0f});
    writer.write(Health{.life = 10.0f});
    std::cout << std::endl;
}

bool chech_if_event(entity::EventReader<Health> reader) {
    std::cout << "chech_if_event" << std::endl;
    std::cout << std::endl;
    return !reader.empty();
}

void read_event(entity::EventReader<Health> reader) {
    std::cout << "read_event" << std::endl;
    for (auto health : reader.read()) {
        std::cout << "event: " << health.life << std::endl;
    }
    std::cout << std::endl;
}

struct TestPlugin : entity::Plugin {
    void build(entity::App& app) override {
        app.add_system(print);
    };
};

int main() {
    entity::App app;
    app.run_system(start).run_system(print);
    app.run_system(create_child).run_system(print);
    app.run_system(despawn).run_system(print);
    app.run_system(add_res).run_system(print_res);
    app.run_system(change_res).run_system(print_res);
    app.run_system(access_not_exist_res);
    app.run_system(add_not_exist_res).run_system(access_not_exist_res);
    app.run_system(remove_not_exist_res).run_system(access_not_exist_res);
    app.run_system(write_event);
    app.run_system(read_event, chech_if_event);
    app.run_system(access_no_entity_health);

    std::cout << std::boolalpha
              << entity::queries_contrary<
                     entity::Query<std::tuple<entt::entity, const Health>,
                                   std::tuple<Position>>,
                     entity::Query<std::tuple<entt::entity, Health, Position>,
                                   std::tuple<>>>()
                     .value()
              << std::endl;

    app.add_system(print).run();
    app.add_system(read_event, chech_if_event).run();
    app.add_plugin(new TestPlugin()).run();

    return 0;
}