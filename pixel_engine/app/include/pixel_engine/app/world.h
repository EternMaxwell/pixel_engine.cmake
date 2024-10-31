#pragma once

#include <deque>
#include <entt/entity/registry.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include "command.h"
#include "event.h"
#include "query.h"
#include "resource.h"
#include "system.h"
#include "tools.h"

namespace pixel_engine {
namespace app {

struct World {
    entt::registry m_registry;
    std::unordered_map<entt::entity, std::unordered_set<entt::entity>>
        m_entity_tree;
    std::unordered_map<const type_info*, std::shared_ptr<void>> m_resources;
    std::unordered_map<const type_info*, std::shared_ptr<std::deque<Event>>>
        m_events;
};

template <typename T>
struct BasicSystem;
template <typename... Args>
struct System;

struct SubApp {
    World m_world;
    std::vector<Command> m_commands;
    std::vector<std::shared_ptr<BasicSystem<void>>> m_state_updates;

    template <typename T, typename... Args>
    T tuple_get(std::tuple<Args...> tuple) {
        using namespace app_tools;
        if constexpr (tuple_contain<T, std::tuple<Args...>>::value) {
            return std::get<T>(tuple);
        } else {
            return T();
        }
    }

    template <template <typename...> typename T, typename... Args>
    constexpr auto tuple_get_template(std::tuple<Args...> tuple) {
        using namespace app_tools;
        if constexpr (tuple_contain_template<T, std::tuple<Args...>>) {
            return std::get<tuple_template_index<T, std::tuple<Args...>>::index(
            )>(tuple);
        } else {
            return T();
        }
    }

   public:
    template <typename T>
    struct value_type {
        static T get(SubApp* src, SubApp* dst) {
            if constexpr (app_tools::is_template_of<Query, T>::value) {
                return T(src->m_world.m_registry);
            } else {
                static_assert(false, "The type is not supported in SubApp.");
            }
        }
    };

    template <>
    struct value_type<Command> {
        static Command get(SubApp* src, SubApp* dsta) {
            auto dst = &dsta->m_world;
            Command cmd(
                &dst->m_registry, &dst->m_resources, &dst->m_entity_tree
            );
            dsta->m_commands.push_back(cmd);
            return cmd;
        }
    };

    template <typename... Gets, typename... Withs, typename... Exs>
    struct value_type<Query<Get<Gets...>, With<Withs...>, Without<Exs...>>> {
        static Query<Get<Gets...>, With<Withs...>, Without<Exs...>> get(
            SubApp* src, SubApp* dst
        ) {
            return Query<Get<Gets...>, With<Withs...>, Without<Exs...>>(
                dst->m_world.m_registry
            );
        }
    };

    template <typename... Gets, typename... Withs, typename... Exs>
    struct value_type<Extract<Get<Gets...>, With<Withs...>, Without<Exs...>>> {
        static Extract<Get<Gets...>, With<Withs...>, Without<Exs...>> get(
            SubApp* src, SubApp* dst
        ) {
            return Extract<Get<Gets...>, With<Withs...>, Without<Exs...>>(
                src->m_world.m_registry
            );
        }
    };

    template <typename Res>
    struct value_type<Resource<Res>> {
        static Resource<Res> get(SubApp* src, SubApp* dst) {
            if (src->m_world.m_resources.find(&typeid(Res)) !=
                src->m_world.m_resources.end()) {
                return Resource<Res>(src->m_world.m_resources[&typeid(Res)]);
            } else {
                return Resource<Res>();
            }
        }
    };

    template <typename Evt>
    struct value_type<EventWriter<Evt>> {
        static EventWriter<Evt> get(SubApp* src, SubApp* dst) {
            if (dst->m_world.m_events.find(&typeid(Evt)) ==
                dst->m_world.m_events.end()) {
                dst->m_world.m_events[&typeid(Evt)] =
                    std::make_shared<std::deque<Event>>();
            }
            return EventWriter<Evt>(dst->m_world.m_events[&typeid(Evt)]);
        }
    };

    template <typename Evt>
    struct value_type<EventReader<Evt>> {
        static EventReader<Evt> get(SubApp* src, SubApp* dst) {
            if (src->m_world.m_events.find(&typeid(Evt)) ==
                src->m_world.m_events.end()) {
                src->m_world.m_events[&typeid(Evt)] =
                    std::make_shared<std::deque<Event>>();
            }
            return EventReader<Evt>(src->m_world.m_events[&typeid(Evt)]);
        }
    };

    SubApp() = default;

   private:
    /*!
     * @brief This is where the systems get their parameters by
     * type. All possible type should be handled here.
     */
    template <typename... Args>
    std::tuple<Args...> get_values() {
        return std::make_tuple(value_type<Args>::get(this, this)...);
    }

    /*! @brief Get a command object on this app.
     * @return The command object.
     */
    Command command();

    template <typename T>
    std::shared_ptr<System<Resource<State<T>>, Resource<NextState<T>>>>
    get_state_update() {
        return std::make_shared<
            System<Resource<State<T>>, Resource<NextState<T>>>>(
            [](Resource<State<T>> state, Resource<NextState<T>> next_state) {
                state->m_state      = next_state->m_state;
                state->just_created = false;
            }
        );
    }

   public:
    /*! @brief Insert a state.
     * If the state already exists, nothing will happen.
     * @tparam T The type of the state.
     * @param state The state value to be set when inserted.
     * @return The App object itself.
     */
    template <typename T>
    SubApp& insert_state(T state) {
        Command cmd = command();
        cmd.insert_resource(State(state));
        cmd.insert_resource(NextState(state));
        m_state_updates.push_back(get_state_update<T>());
        return *this;
    }

    /*! @brief Insert a state using default values.
     * If the state already exists, nothing will happen.
     * @tparam T The type of the state.
     * @return The App object itself.
     */
    template <typename T>
    SubApp& init_state() {
        Command cmd = command();
        cmd.init_resource<State<T>>();
        cmd.init_resource<NextState<T>>();
        m_state_updates.push_back(get_state_update<T>());
        return *this;
    }

    /**
     * @brief Insert a resource.
     *
     * @tparam T The type of the resource.
     * @param res The resource to be inserted.
     * @return World& The App object itself.
     */
    template <typename T>
    SubApp& insert_resource(T res) {
        Command cmd = command();
        cmd.insert_resource(res);
        return *this;
    }

    /**
     * @brief Insert a resource.
     *
     * @tparam T The type of the resource.
     * @tparam Args The types of the arguments to be passed to the constructor
     * of T
     * @param args The arguments to be passed to the constructor of T
     * @return World& The App object itself.
     */
    template <typename T, typename... Args>
    SubApp& insert_resource(Args... args) {
        Command cmd = command();
        cmd.insert_resource<T>(args...);
        return *this;
    }

    /**
     * @brief Insert a resource using default values.
     *
     * @tparam T The type of the resource.=
     * @return World& The App object itself.
     */
    template <typename T>
    SubApp& init_resource() {
        Command cmd = command();
        cmd.init_resource<T>();
        return *this;
    }

    SubApp& emplace_resource(const type_info* type, std::shared_ptr<void> res) {
        m_world.m_resources.insert({type, res});
        return *this;
    }

    template <typename T>
    auto get_resource() {
        return value_type<Resource<T>>::get(this);
    }

    void tick_events();

    void update_states();

    void end_commands();

    template <typename T>
    T get_app_parameter() {
        return value_type<T>::get(this, this);
    }

    template <typename Ret, typename... Args>
    Ret run_system_v(std::function<Ret(Args...)> func) {
        auto values = get_values<Args...>();
        return std::apply(func, values);
    }

    template <typename Ret, typename... Args>
    Ret run_system_v(Ret (*func)(Args...)) {
        auto values = get_values<Args...>();
        return std::apply(func, values);
    }
};
}  // namespace app
}  // namespace pixel_engine
