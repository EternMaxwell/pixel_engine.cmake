#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "command.h"
#include "query.h"
#include "resource.h"
#include "tools.h"
#include "world.h"

namespace pixel_engine {
namespace app {
template <typename Ret>
struct BasicSystem {
   protected:
    bool has_command = false;
    bool has_query   = false;
    double avg_time  = 1.0;  // in milliseconds
    std::vector<std::tuple<
        std::vector<const type_info*>,
        std::vector<const type_info*>,
        std::vector<const type_info*>>>
        query_types;
    std::vector<const type_info*> resource_types;
    std::vector<const type_info*> resource_const;
    std::vector<const type_info*> event_read_types;
    std::vector<const type_info*> event_write_types;
    std::vector<const type_info*> state_types;
    std::vector<const type_info*> next_state_types;

    template <typename Arg>
    struct info_add {
        static void add(std::vector<const type_info*>& infos) {
            infos.push_back(&typeid(std::remove_const_t<Arg>));
        }
    };

    template <typename Arg>
    struct const_infos_adder {
        static void add(std::vector<const type_info*>& infos) {
            if constexpr (std::is_const_v<Arg>)
                infos.push_back(&typeid(std::remove_const_t<Arg>));
        }
    };

    template <typename Arg>
    struct non_const_infos_adder {
        static void add(std::vector<const type_info*>& infos) {
            if constexpr (!std::is_const_v<Arg>) infos.push_back(&typeid(Arg));
        }
    };

    template <typename T>
    struct infos_adder {};

    template <typename... Includes, typename... Withs, typename... Excludes>
    struct infos_adder<
        Query<Get<Includes...>, With<Withs...>, Without<Excludes...>>> {
        static void add(
            std::vector<std::tuple<
                std::vector<const type_info*>,
                std::vector<const type_info*>,
                std::vector<const type_info*>>>& query_types,
            std::vector<const type_info*>& resource_types,
            std::vector<const type_info*>& resource_const,
            std::vector<const type_info*>& event_read_types,
            std::vector<const type_info*>& event_write_types,
            std::vector<const type_info*>& state_types,
            std::vector<const type_info*>& next_state_types
        ) {
            std::vector<const type_info*> query_include_types,
                query_exclude_types, query_include_const;
            (non_const_infos_adder<Includes>::add(query_include_types), ...);
            (const_infos_adder<Includes>::add(query_include_const), ...);
            (info_add<Withs>::add(query_include_const), ...);
            (info_add<Excludes>::add(query_exclude_types), ...);
            query_types.push_back(std::make_tuple(
                query_include_types, query_include_const, query_exclude_types
            ));
        }
    };

    template <typename... Includes, typename... Excludes, typename T>
    struct infos_adder<Query<Get<Includes...>, Without<Excludes...>, T>> {
        static void add(
            std::vector<std::tuple<
                std::vector<const type_info*>,
                std::vector<const type_info*>,
                std::vector<const type_info*>>>& query_types,
            std::vector<const type_info*>& resource_types,
            std::vector<const type_info*>& resource_const,
            std::vector<const type_info*>& event_read_types,
            std::vector<const type_info*>& event_write_types,
            std::vector<const type_info*>& state_types,
            std::vector<const type_info*>& next_state_types
        ) {
            std::vector<const type_info*> query_include_types,
                query_exclude_types, query_include_const;
            (non_const_infos_adder<Includes>::add(query_include_types), ...);
            (const_infos_adder<Includes>::add(query_include_const), ...);
            (info_add<Excludes>::add(query_exclude_types), ...);
            query_types.push_back(std::make_tuple(
                query_include_types, query_include_const, query_exclude_types
            ));
        }
    };

    template <typename T>
    struct infos_adder<Resource<T>> {
        static void add(
            std::vector<std::tuple<
                std::vector<const type_info*>,
                std::vector<const type_info*>,
                std::vector<const type_info*>>>& query_types,
            std::vector<const type_info*>& resource_types,
            std::vector<const type_info*>& resource_const,
            std::vector<const type_info*>& event_read_types,
            std::vector<const type_info*>& event_write_types,
            std::vector<const type_info*>& state_types,
            std::vector<const type_info*>& next_state_types
        ) {
            if constexpr (std::is_const_v<T>)
                info_add<T>().add(resource_const);
            else
                info_add<T>().add(resource_types);
        }
    };

    template <typename T>
    struct infos_adder<EventReader<T>> {
        static void add(
            std::vector<std::tuple<
                std::vector<const type_info*>,
                std::vector<const type_info*>,
                std::vector<const type_info*>>>& query_types,
            std::vector<const type_info*>& resource_types,
            std::vector<const type_info*>& resource_const,
            std::vector<const type_info*>& event_read_types,
            std::vector<const type_info*>& event_write_types,
            std::vector<const type_info*>& state_types,
            std::vector<const type_info*>& next_state_types
        ) {
            info_add<T>().add(event_read_types);
        }
    };

    template <typename T>
    struct infos_adder<EventWriter<T>> {
        static void add(
            std::vector<std::tuple<
                std::vector<const type_info*>,
                std::vector<const type_info*>,
                std::vector<const type_info*>>>& query_types,
            std::vector<const type_info*>& resource_types,
            std::vector<const type_info*>& resource_const,
            std::vector<const type_info*>& event_read_types,
            std::vector<const type_info*>& event_write_types,
            std::vector<const type_info*>& state_types,
            std::vector<const type_info*>& next_state_types
        ) {
            info_add<T>().add(event_write_types);
        }
    };

    template <typename T>
    struct infos_adder<State<T>> {
        static void add(
            std::vector<std::tuple<
                std::vector<const type_info*>,
                std::vector<const type_info*>,
                std::vector<const type_info*>>>& query_types,
            std::vector<const type_info*>& resource_types,
            std::vector<const type_info*>& resource_const,
            std::vector<const type_info*>& event_read_types,
            std::vector<const type_info*>& event_write_types,
            std::vector<const type_info*>& state_types,
            std::vector<const type_info*>& next_state_types
        ) {
            info_add<T>().add(state_types);
        }
    };

    template <typename T>
    struct infos_adder<NextState<T>> {
        static void add(
            std::vector<std::tuple<
                std::vector<const type_info*>,
                std::vector<const type_info*>,
                std::vector<const type_info*>>>& query_types,
            std::vector<const type_info*>& resource_types,
            std::vector<const type_info*>& resource_const,
            std::vector<const type_info*>& event_read_types,
            std::vector<const type_info*>& event_write_types,
            std::vector<const type_info*>& state_types,
            std::vector<const type_info*>& next_state_types
        ) {
            info_add<T>().add(next_state_types);
        }
    };

    template <typename Arg, typename... Args>
    void add_infos_inernal() {
        using namespace app_tools;
        if constexpr (std::is_same_v<Arg, Command>) {
            has_command = true;
        } else if constexpr (is_template_of<Query, Arg>::value) {
            has_query = true;
            infos_adder<Arg>().add(
                query_types, resource_types, resource_const, event_read_types,
                event_write_types, state_types, next_state_types
            );
        } else {
            infos_adder<Arg>().add(
                query_types, resource_types, resource_const, event_read_types,
                event_write_types, state_types, next_state_types
            );
        }
        if constexpr (sizeof...(Args) > 0) add_infos<Args...>();
    }

    template <typename... Args>
    void add_infos() {
        if constexpr (sizeof...(Args) > 0) add_infos_inernal<Args...>();
    }

   public:
    bool contrary_to(std::shared_ptr<BasicSystem>& other) {
        if (has_command && (other->has_command || other->has_query))
            return true;
        if (has_query && other->has_command) return true;
        for (auto& [query_include_types, query_include_const, query_exclude_types] :
             query_types) {
            for (auto& [other_query_include_types, other_query_include_const, other_query_exclude_types] :
                 other->query_types) {
                bool this_exclude_other = false;
                for (auto type : query_exclude_types) {
                    if (std::find(
                            other_query_include_types.begin(),
                            other_query_include_types.end(), type
                        ) != other_query_include_types.end()) {
                        this_exclude_other = true;
                    }
                    if (std::find(
                            other_query_include_const.begin(),
                            other_query_include_const.end(), type
                        ) != other_query_include_const.end()) {
                        this_exclude_other = true;
                    }
                }
                if (this_exclude_other) continue;
                bool other_exclude_this = false;
                for (auto type : other_query_exclude_types) {
                    if (std::find(
                            query_include_types.begin(),
                            query_include_types.end(), type
                        ) != query_include_types.end()) {
                        other_exclude_this = true;
                    }
                    if (std::find(
                            query_include_const.begin(),
                            query_include_const.end(), type
                        ) != query_include_const.end()) {
                        other_exclude_this = true;
                    }
                }
                if (other_exclude_this) continue;
                for (auto type : query_include_types) {
                    if (std::find(
                            other_query_include_types.begin(),
                            other_query_include_types.end(), type
                        ) != other_query_include_types.end()) {
                        return true;
                    }
                    if (std::find(
                            other_query_include_const.begin(),
                            other_query_include_const.end(), type
                        ) != other_query_include_const.end()) {
                        return true;
                    }
                }
            }
        }

        bool resource_one_empty =
            resource_types.empty() || other->resource_types.empty();
        bool resource_contrary = !resource_one_empty;
        if (!resource_one_empty) {
            for (auto type : resource_types) {
                if (std::find(
                        other->resource_const.begin(),
                        other->resource_const.end(), type
                    ) != other->resource_const.end()) {
                    resource_contrary = true;
                }
                if (std::find(
                        other->resource_types.begin(),
                        other->resource_types.end(), type
                    ) != other->resource_types.end()) {
                    resource_contrary = true;
                }
            }
            for (auto type : other->resource_types) {
                if (std::find(
                        resource_const.begin(), resource_const.end(), type
                    ) != resource_const.end()) {
                    resource_contrary = true;
                }
                if (std::find(
                        resource_types.begin(), resource_types.end(), type
                    ) != resource_types.end()) {
                    resource_contrary = true;
                }
            }
        }
        if (resource_contrary) return true;

        bool event_contrary = false;
        for (auto type : event_write_types) {
            if (std::find(
                    other->event_write_types.begin(),
                    other->event_write_types.end(), type
                ) != other->event_write_types.end()) {
                event_contrary = true;
            }
            if (std::find(
                    other->event_read_types.begin(),
                    other->event_read_types.end(), type
                ) != other->event_read_types.end()) {
                event_contrary = true;
            }
        }
        for (auto type : other->event_write_types) {
            if (std::find(
                    event_write_types.begin(), event_write_types.end(), type
                ) != event_write_types.end()) {
                event_contrary = true;
            }
            if (std::find(
                    event_read_types.begin(), event_read_types.end(), type
                ) != event_read_types.end()) {
                event_contrary = true;
            }
        }
        for (auto type : event_read_types) {
            if (std::find(
                    other->event_read_types.begin(),
                    other->event_read_types.end(), type
                ) != other->event_read_types.end()) {
                event_contrary = true;
            }
        }
        if (event_contrary) return true;

        bool state_contrary = false;
        for (auto type : next_state_types) {
            if (std::find(
                    other->next_state_types.begin(),
                    other->next_state_types.end(), type
                ) != other->next_state_types.end()) {
                state_contrary = true;
            }
            if (std::find(
                    other->state_types.begin(), other->state_types.end(), type
                ) != other->state_types.end()) {
                state_contrary = true;
            }
        }
        for (auto type : other->next_state_types) {
            if (std::find(
                    next_state_types.begin(), next_state_types.end(), type
                ) != next_state_types.end()) {
                state_contrary = true;
            }
            if (std::find(state_types.begin(), state_types.end(), type) !=
                state_types.end()) {
                state_contrary = true;
            }
        }
        return state_contrary;
    }
    void print_info_types_name() {
        std::cout << "has command: " << has_command << std::endl;

        for (auto& [query_include_types, query_include_const, query_exclude_types] :
             query_types) {
            std::cout << "query_include_types: ";
            for (auto type : query_include_types)
                std::cout << type->name() << " ";
            std::cout << std::endl;

            std::cout << "query_include_const: ";
            for (auto type : query_include_const)
                std::cout << type->name() << " ";
            std::cout << std::endl;

            std::cout << "query_exclude_types: ";
            for (auto type : query_exclude_types)
                std::cout << type->name() << " ";
            std::cout << std::endl;
        }

        std::cout << "resource_types: ";
        for (auto type : resource_types) std::cout << type->name() << " ";
        std::cout << std::endl;

        std::cout << "resource_const: ";
        for (auto type : resource_const) std::cout << type->name() << " ";
        std::cout << std::endl;

        std::cout << "event_read_types: ";
        for (auto type : event_read_types) std::cout << type->name() << " ";
        std::cout << std::endl;

        std::cout << "event_write_types: ";
        for (auto type : event_write_types) std::cout << type->name() << " ";
        std::cout << std::endl;

        std::cout << "state_types: ";
        for (auto type : state_types) std::cout << type->name() << " ";
        std::cout << std::endl;

        std::cout << "next_state_types: ";
        for (auto type : next_state_types) std::cout << type->name() << " ";
        std::cout << std::endl;
    }
    const double get_avg_time() { return avg_time; }
    virtual Ret run(SubApp* src, SubApp* dst) = 0;
};

struct SubAppParaRetriever {
    template <typename T>
    static T get(SubApp* src, SubApp* dst) {
        return SubApp::value_type<T>::get(src, dst);
    }
};

template <typename Ret, typename... Args>
struct SystemFunctionInvoker {
    static Ret invoke(
        std::function<Ret(Args...)> func, SubApp* src, SubApp* dst
    ) {
        return func(SubAppParaRetriever::get<Args>(src, dst)...);
    }
};

template <typename... Args>
struct System : public BasicSystem<void> {
   private:
    std::function<void(Args...)> func;

   public:
    System(std::function<void(Args...)> func)
        : BasicSystem<void>(), func(func) {
        add_infos<Args...>();
    }
    // System(void (*func)(Args...)) : BasicSystem<void>(), func(func) {
    //     add_infos<Args...>();
    // }
    void run(SubApp* src, SubApp* dst) override {
        auto start = std::chrono::high_resolution_clock::now();
        SystemFunctionInvoker<void, Args...>::invoke(func, src, dst);
        auto end = std::chrono::high_resolution_clock::now();
        auto delta =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                .count();
        avg_time = delta * 0.1 + avg_time * 0.9;
    }
};

template <typename... Args>
struct Condition : public BasicSystem<bool> {
   private:
    std::function<bool(Args...)> func;

   public:
    Condition(std::function<bool(Args...)> func)
        : BasicSystem<bool>(), func(func) {
        add_infos<Args...>();
    }
    Condition(bool (*func)(Args...)) : BasicSystem<bool>(), func(func) {
        add_infos<Args...>();
    }
    bool run(SubApp* src, SubApp* dst) override {
        return SystemFunctionInvoker<bool, Args...>::invoke(func, src, dst);
    }
};

}  // namespace app
}  // namespace pixel_engine