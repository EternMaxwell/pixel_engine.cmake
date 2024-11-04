#pragma once

#include <entt/entity/registry.hpp>

#include "command.h"
#include "event.h"
#include "query.h"
#include "resource.h"
#include "world.h"

namespace pixel_engine::app {
struct App;
struct SubApp {
    template <typename T>
    struct value_type {
        static T get(SubApp& app) {
            if constexpr (app_tools::is_template_of<Query, T>::value) {
                return T(app.m_world.m_registry);
            } else if constexpr (app_tools::is_template_of<Extract, T>::value) {
                return T(app.m_world.m_registry);
            } else {
                static_assert(false, "Not allowed type");
            }
        }
        static T get(SubApp& src, SubApp& dst) {
            if constexpr (app_tools::is_template_of<Query, T>::value) {
                return T(dst.m_world.m_registry);
            } else if constexpr (app_tools::is_template_of<Extract, T>::value) {
                return T(src.m_world.m_registry);
            } else {
                static_assert(false, "Not allowed type");
            }
        }
    };

    template <typename ResT>
    struct value_type<Res<ResT>> {
        static Res<ResT> get(SubApp& app) {
            return Res<ResT>(app.m_world.m_resources[&typeid(ResT)].get());
        }
        static Res<ResT> get(SubApp& src, SubApp& dst) {
            return Res<ResT>(dst.m_world.m_resources[&typeid(ResT)].get());
        }
    };

    template <typename ResT>
    struct value_type<ResMut<ResT>> {
        static ResMut<ResT> get(SubApp& app) {
            return ResMut<ResT>(app.m_world.m_resources[&typeid(ResT)].get());
        }
        static ResMut<ResT> get(SubApp& src, SubApp& dst) {
            return ResMut<ResT>(dst.m_world.m_resources[&typeid(ResT)].get());
        }
    };

    template <>
    struct value_type<Command> {
        static Command get(SubApp& app) {
            app.m_command_cache.emplace_back(Command(&app.m_world));
            return app.m_command_cache.back();
        }
        static Command get(SubApp& src, SubApp& dst) {
            dst.m_command_cache.emplace_back(Command(&dst.m_world));
            return dst.m_command_cache.back();
        }
    };

    template <typename T>
    struct value_type<EventReader<T>> {
        static EventReader<T> get(SubApp& app) {
            return EventReader<T>(app.m_world.m_event_queues[&typeid(T)].get());
        }
        static EventReader<T> get(SubApp& src, SubApp& dst) {
            return EventReader<T>(src.m_world.m_event_queues[&typeid(T)].get());
        }
    };

    template <typename T>
    struct value_type<EventWriter<T>> {
        static EventWriter<T> get(SubApp& app) {
            return EventWriter<T>(app.m_world.m_event_queues[&typeid(T)].get());
        }
        static EventWriter<T> get(SubApp& src, SubApp& dst) {
            return EventWriter<T>(dst.m_world.m_event_queues[&typeid(T)].get());
        }
    };

    template <typename... Gs, typename... Ws, typename... WOs>
    struct value_type<Query<Get<Gs...>, With<Ws...>, Without<WOs...>>> {
        static Query<Get<Gs...>, With<Ws...>, Without<WOs...>> get(SubApp& app
        ) {
            return Query<Get<Gs...>, With<Ws...>, Without<WOs...>>(
                app.m_world.m_registry
            );
        }
        static Query<Get<Gs...>, With<Ws...>, Without<WOs...>> get(
            SubApp& src, SubApp& dst
        ) {
            return Query<Get<Gs...>, With<Ws...>, Without<WOs...>>(
                dst.m_world.m_registry
            );
        }
    };

    template <typename... Gs, typename... Ws, typename... WOs>
    struct value_type<Extract<Get<Gs...>, With<Ws...>, Without<WOs...>>> {
        static Extract<Get<Gs...>, With<Ws...>, Without<WOs...>> get(SubApp& app
        ) {
            return Extract<Get<Gs...>, With<Ws...>, Without<WOs...>>(
                app.m_world.m_registry
            );
        }
        static Extract<Get<Gs...>, With<Ws...>, Without<WOs...>> get(
            SubApp& src, SubApp& dst
        ) {
            return Extract<Get<Gs...>, With<Ws...>, Without<WOs...>>(
                src.m_world.m_registry
            );
        }
    };

   public:
    void tick_events() {
        for (auto& [type, queue] : m_world.m_event_queues) {
            queue->tick();
        }
    }
    void end_commands() {
        for (auto& command : m_command_cache) {
            command.end();
        }
        m_command_cache.clear();
    }
    template <typename... Args>
    void insert_resource(Args&&... resource) {
        Command command(&m_world);
        command.insert_resource(std::forward<Args>(resource)...);
    }
    template <typename T>
    void init_resource() {
        Command command(&m_world);
        command.init_resource<T>();
    }
    template <typename T>
    void insert_state(T&& state) {
        Command command(&m_world);
        command.insert_resource(State<T>(std::forward<T>(state)));
        command.insert_resource(NextState<T>(std::forward<T>(state)));
        m_state_updates.push_back(
            std::make_unique<System<State<T>, NextState<T>>>(
                [this](State<T>& state, NextState<T>& next_state) {
                    state.m_state      = next_state.m_state;
                    state.just_created = false;
                }
            )
        );
    }
    template <typename T>
    void init_state() {
        Command command(&m_world);
        command.init_resource(State<T>());
        command.init_resource(NextState<T>());
    }

    void update_states() {
        for (auto& update : m_state_updates) {
            update->run(this, this);
        }
    }

   private:
    World m_world;
    std::vector<Command> m_command_cache;
    std::vector<std::unique_ptr<BasicSystem<void>>> m_state_updates;

    friend struct App;
};
}  // namespace pixel_engine::app