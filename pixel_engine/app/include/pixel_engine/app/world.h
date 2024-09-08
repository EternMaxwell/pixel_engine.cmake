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

namespace pixel_engine {
namespace app {

struct World {
   private:
    entt::registry m_registry;
    std::unordered_map<entt::entity, std::unordered_set<entt::entity>>
        m_entity_tree;
    std::unordered_map<size_t, std::shared_ptr<void>> m_resources;
    std::unordered_map<size_t, std::shared_ptr<std::deque<Event>>> m_events;

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
            return std::get<
                tuple_template_index<T, std::tuple<Args...>>::index()>(tuple);
        } else {
            return T();
        }
    }

    template <typename T>
    struct value_type {
        static T get(World* app) { static_assert(1, "value type not valid."); }
    };

    template <>
    struct value_type<Command> {
        static Command get(World* app) {
            return Command(
                &app->m_registry, &app->m_resources, &app->m_entity_tree);
        }
    };

    template <typename... Gets, typename... Withs, typename... Exs>
    struct value_type<Query<Get<Gets...>, With<Withs...>, Without<Exs...>>> {
        static Query<Get<Gets...>, With<Withs...>, Without<Exs...>> get(
            World* app) {
            return Query<Get<Gets...>, With<Withs...>, Without<Exs...>>(
                app->m_registry);
        }
    };

    template <typename Res>
    struct value_type<Resource<Res>> {
        static Resource<Res> get(World* app) {
            if (app->m_resources.find(typeid(Res).hash_code()) !=
                app->m_resources.end()) {
                return Resource<Res>(
                    &app->m_resources[typeid(Res).hash_code()]);
            } else {
                return Resource<Res>();
            }
        }
    };

    template <typename Evt>
    struct value_type<EventWriter<Evt>> {
        static EventWriter<Evt> get(World* app) {
            if (app->m_events.find(typeid(Evt).hash_code()) ==
                app->m_events.end()) {
                app->m_events[typeid(Evt).hash_code()] =
                    std::make_shared<std::deque<Event>>();
            }
            return EventWriter<Evt>(app->m_events[typeid(Evt).hash_code()]);
        }
    };

    template <typename Evt>
    struct value_type<EventReader<Evt>> {
        static EventReader<Evt> get(World* app) {
            if (app->m_events.find(typeid(Evt).hash_code()) ==
                app->m_events.end()) {
                app->m_events[typeid(Evt).hash_code()] =
                    std::make_shared<std::deque<Event>>();
            }
            return EventReader<Evt>(app->m_events[typeid(Evt).hash_code()]);
        }
    };

    /*!
     * @brief This is where the systems get their parameters by
     * type. All possible type should be handled here.
     */
    template <typename... Args>
    std::tuple<Args...> get_values() {
        return std::make_tuple(value_type<Args>::get(this)...);
    }

    /*! @brief Get a command object on this app.
     * @return The command object.
     */
    Command command() {
        return Command(&m_registry, &m_resources, &m_entity_tree);
    }

   public:
    std::mutex m_system_mutex;

    /*! @brief Insert a state.
     * If the state already exists, nothing will happen.
     * @tparam T The type of the state.
     * @param state The state value to be set when inserted.
     * @return The App object itself.
     */
    template <typename T>
    World& insert_state(T state) {
        Command cmd = command();
        cmd.insert_resource(State(state));
        cmd.insert_resource(NextState(state));
        return *this;
    }

    /*! @brief Insert a state using default values.
     * If the state already exists, nothing will happen.
     * @tparam T The type of the state.
     * @return The App object itself.
     */
    template <typename T>
    World& init_state() {
        Command cmd = command();
        cmd.init_resource<State<T>>();
        cmd.init_resource<NextState<T>>();
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
    World& insert_resource(T res) {
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
    World& insert_resource(Args... args) {
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
    World& init_resource() {
        Command cmd = command();
        cmd.init_resource<T>();
        return *this;
    }

    /*! @brief Run a system.
     * @tparam Args The types of the arguments for the system.
     * @param func The system to be run.
     * @return The App object itself.
     */
    template <typename... Args>
    World& run_system(std::function<void(Args...)> func) {
        std::apply(func, get_values<Args...>());
        return *this;
    }

    /*! @brief Run a system.
     * @tparam Args The types of the arguments for the system.
     * @param func The system to be run.
     * @return The App object itself.
     */
    template <typename... Args>
    World& run_system(void (*func)(Args...)) {
        std::apply(func, get_values<Args...>());
        return *this;
    }

    void tick_events() {
        for (auto& [key, events] : m_events) {
            while (!events->empty()) {
                auto& evt = events->front();
                if (evt.ticks >= 1) {
                    events->pop_front();
                } else {
                    break;
                }
            }
            for (auto& evt : *events) {
                evt.ticks++;
            }
        }
    }

    /*! @brief Run a system.
     * @tparam Args1 The types of the arguments for the system.
     * @tparam Args2 The types of the arguments for the condition.
     * @param func The system to be run.
     * @param condition The condition for the system to be run.
     * @return The App object itself.
     */
    template <typename T, typename... Args>
    T run_system_v(std::function<T(Args...)> func) {
        return std::apply(func, get_values<Args...>());
    }

    /*! @brief Run a system.
     * @tparam Args1 The types of the arguments for the system.
     * @tparam Args2 The types of the arguments for the condition.
     * @param func The system to be run.
     * @param condition The condition for the system to be run.
     * @return The App object itself.
     */
    template <typename T, typename... Args>
    T run_system_v(T (*func)(Args...)) {
        return std::apply(func, get_values<Args...>());
    }
};
}  // namespace app
}  // namespace pixel_engine
