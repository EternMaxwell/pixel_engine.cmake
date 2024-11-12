#pragma once

#include <spdlog/spdlog.h>

#include <entt/entity/registry.hpp>

#include "command.h"
#include "event.h"
#include "query.h"
#include "resource.h"
#include "system.h"
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
            return Res<ResT>(app.m_world
                                 .m_resources[std::type_index(
                                     typeid(std::remove_const_t<ResT>)
                                 )]
                                 .get());
        }
        static Res<ResT> get(SubApp& src, SubApp& dst) { return get(dst); }
    };

    template <typename ResT>
    struct value_type<ResMut<ResT>> {
        static ResMut<ResT> get(SubApp& app) {
            return ResMut<ResT>(app.m_world
                                    .m_resources[std::type_index(
                                        typeid(std::remove_const_t<ResT>)
                                    )]
                                    .get());
        }
        static ResMut<ResT> get(SubApp& src, SubApp& dst) { return get(dst); }
    };

    template <>
    struct value_type<Command> {
        static Command get(SubApp& app) {
            app.m_command_cache.emplace_back(Command(&app.m_world));
            return app.m_command_cache.back();
        }
        static Command get(SubApp& src, SubApp& dst) { return get(dst); }
    };

    template <typename T>
    struct value_type<EventReader<T>> {
        static EventReader<T> get(SubApp& app) {
            return EventReader<T>(
                app.m_world.m_event_queues[std::type_index(typeid(T))].get()
            );
        }
        static EventReader<T> get(SubApp& src, SubApp& dst) { return get(src); }
    };

    template <typename T>
    struct value_type<EventWriter<T>> {
        static EventWriter<T> get(SubApp& app) {
            return EventWriter<T>(
                app.m_world.m_event_queues[std::type_index(typeid(T))].get()
            );
        }
        static EventWriter<T> get(SubApp& src, SubApp& dst) { return get(dst); }
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
            return get(dst);
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
            return get(src);
        }
    };

   public:
    void tick_events();
    void end_commands();
    void update_states();

    template <typename T, typename... Args>
    void emplace_resource(Args&&... args) {
        Command command(&m_world);
        command.emplace_resource<T>(std::forward<Args>(args)...);
    }
    template <typename T>
    void insert_resource(T&& res) {
        Command command(&m_world);
        command.insert_resource(std::forward<T>(res));
    }
    template <typename T>
    void init_resource() {
        Command command(&m_world);
        command.init_resource<T>();
    }
    template <typename T>
    void insert_state(T&& state) {
        if (m_world.m_resources.find(std::type_index(typeid(State<T>))) !=
                m_world.m_resources.end() ||
            m_world.m_resources.find(std::type_index(typeid(NextState<T>))) !=
                m_world.m_resources.end()) {
            spdlog::warn("State already exists.");
            return;
        }
        Command command(&m_world);
        command.insert_resource(State<T>(std::forward<T>(state)));
        command.insert_resource(NextState<T>(std::forward<T>(state)));
        m_state_updates.push_back(
            std::make_unique<System<ResMut<State<T>>, Res<NextState<T>>>>(
                [](ResMut<State<T>> state, Res<NextState<T>> next_state) {
                    state->m_state      = next_state->m_state;
                    state->just_created = false;
                }
            )
        );
    }
    template <typename T>
    void init_state() {
        if (m_world.m_resources.find(std::type_index(typeid(State<T>))) !=
                m_world.m_resources.end() ||
            m_world.m_resources.find(std::type_index(typeid(NextState<T>))) !=
                m_world.m_resources.end()) {
            spdlog::warn("State already exists.");
            return;
        }
        Command command(&m_world);
        command.init_resource<State<T>>();
        command.init_resource<NextState<T>>();
        m_state_updates.push_back(
            std::make_unique<System<ResMut<State<T>>, Res<NextState<T>>>>(
                [](ResMut<State<T>> state, Res<NextState<T>> next_state) {
                    state->m_state      = next_state->m_state;
                    state->just_created = false;
                }
            )
        );
    }
    template <typename T>
    void add_event() {
        if (m_world.m_event_queues.find(std::type_index(typeid(T))) !=
            m_world.m_event_queues.end())
            return;
        m_world.m_event_queues.emplace(
            std::type_index(typeid(T)), std::make_unique<EventQueue<T>>()
        );
    }

   private:
    World m_world;
    std::vector<Command> m_command_cache;
    std::vector<std::unique_ptr<BasicSystem<void>>> m_state_updates;

    friend struct App;
};
}  // namespace pixel_engine::app