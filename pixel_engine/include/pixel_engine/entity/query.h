#pragma once

#include <any>
#include <deque>
#include <entt/entity/registry.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <pfr.hpp>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_map>

namespace pixel_engine {
    namespace entity {
        template <typename Que, typename Exclude>
        class Query {};

        template <typename... Qus, typename... Exs>
        class Query<std::tuple<Qus...>, std::tuple<Exs...>> {
           private:
            entt::registry& registry;
            decltype(registry.view<Qus...>(entt::exclude_t<Exs...>{})
                         .each()) m_iter;

           public:
            Query(entt::registry& registry) : registry(registry) {
                m_iter =
                    registry.view<Qus...>(entt::exclude_t<Exs...>{}).each();
            }

            class iterator : public std::iterator<std::input_iterator_tag,
                                                  std::tuple<Qus...>> {
               private:
                decltype(registry.view<Qus...>(entt::exclude_t<Exs...>{})
                             .each()) m_full;
                decltype(m_full.begin()) m_iter;

                template <size_t I, typename T, typename... Args>
                std::tuple<Args...> remove_first(std::tuple<T, Args...> tuple) {
                    if constexpr (sizeof...(Args) > I) {
                        return std::tuple_cat(
                            std::make_tuple(std::get<I>(tuple)),
                            remove_first<I++>(tuple));
                    } else {
                        return std::make_tuple(std::get<I>(tuple));
                    }
                }

                template <typename T, typename... Args>
                std::tuple<Args...> remove_first(std::tuple<T, Args...> tuple) {
                    return remove_first<1>(tuple);
                }

               public:
                iterator(decltype(m_full) full)
                    : m_full(full), m_iter(full.begin()) {}

                iterator(decltype(m_full) full, decltype(m_full.begin()) iter)
                    : m_full(full), m_iter(iter) {}
                iterator& operator++() {
                    m_iter++;
                    return *this;
                }
                iterator operator++(int) {
                    iterator tmp = *this;
                    ++(*this);
                    return tmp;
                }
                bool operator==(const iterator& rhs) const {
                    return m_iter == rhs.m_iter;
                }
                bool operator!=(const iterator& rhs) const {
                    return m_iter != rhs.m_iter;
                }
                std::tuple<Qus...> operator*() {
                    // remove the first element of the tuple and return the
                    // rest
                    return remove_first(*m_iter);
                }
                auto begin() { return iterator(m_full, m_full.begin()); }

                auto end() { return iterator(m_full, m_full.end()); }
            };

            /*! @brief Get the iterator for the query.
             * @return The iterator for the query.
             */
            auto iter() {
                return iterator(
                    registry.view<Qus...>(entt::exclude_t<Exs...>{}).each());
            }

            /*! @brief Get the single entity and requaired components.
             * @return An optional of a single tuple of entity and requaired
             * components.
             */
            auto single() {
                auto start = *(iter().begin());
                if (iter().begin() != iter().end()) {
                    return std::optional(start);
                } else {
                    return std::optional<decltype(start)>{};
                }
            }
        };

        template <typename... Qus, typename... Exs>
        class Query<std::tuple<entt::entity, Qus...>, std::tuple<Exs...>> {
           private:
            entt::registry& registry;
            decltype(registry.view<Qus...>(entt::exclude_t<Exs...>{})
                         .each()) m_iter;

           public:
            Query(entt::registry& registry) : registry(registry) {
                m_iter =
                    registry.view<Qus...>(entt::exclude_t<Exs...>{}).each();
            }

            /*! @brief Get the iterator for the query.
             * @return The iterator for the query.
             */
            auto iter() {
                return registry.view<Qus...>(entt::exclude_t<Exs...>{}).each();
            }

            /*! @brief Get the single entity and requaired components.
             * @return An optional of a single tuple of entity and requaired
             * components.
             */
            auto single() {
                auto start = *(registry.view<Qus...>(entt::exclude_t<Exs...>{})
                                   .each()
                                   .begin());
                if (registry.view<Qus...>(entt::exclude_t<Exs...>{})
                        .each()
                        .begin() !=
                    registry.view<Qus...>(entt::exclude_t<Exs...>{})
                        .each()
                        .end()) {
                    return std::optional(start);
                } else {
                    return std::optional<decltype(start)>{};
                }
            }
        };

        template <typename Qu1, typename Qu2, typename... Qus1,
                  typename... Qus2, typename... Exs1, typename... Exs2>
        struct queries_contrary<
            Query<std::tuple<Qu1, Qus1...>, std::tuple<Exs1...>>,
            Query<std::tuple<Qu2, Qus2...>, std::tuple<Exs2...>>> {
            template <typename T, typename... Args>
            bool args_all_const() {
                if constexpr (std::is_const_v<T>) {
                    if constexpr (sizeof...(Args) > 0) {
                        return args_all_const<Args...>();
                    } else {
                        return true;
                    }
                } else {
                    return false;
                }
            }

            template <typename T>
            static bool type_in_group() {
                return false;
            }

            template <typename T, typename First, typename... Args>
            static bool type_in_group() {
                if constexpr (std::is_same_v<std::remove_const_t<T>,
                                             std::remove_const_t<First>>) {
                    return true;
                } else {
                    if constexpr (sizeof...(Args) > 0) {
                        return type_in_group<T, Args...>();
                    } else {
                        return false;
                    }
                }
            }

            template <typename First, typename Second>
            struct first_tuple_contains_any_type_in_second_tuple {
                static const bool value = false;
            };

            template <typename... Args1, typename Args2First, typename... Args2>
            struct first_tuple_contains_any_type_in_second_tuple<
                std::tuple<Args1...>, std::tuple<Args2First, Args2...>> {
                bool value() {
                    if constexpr (sizeof...(Args1) == 0) {
                        return false;
                    } else if (type_in_group<std::remove_const_t<Args2First>,
                                             Args1...>()) {
                        return true;
                    } else {
                        if constexpr (sizeof...(Args2) > 0) {
                            return first_tuple_contains_any_type_in_second_tuple<
                                       std::tuple<Args1...>,
                                       std::tuple<Args2...>>{}
                                .value();
                        } else {
                            return false;
                        }
                    }
                }
            };

            template <typename First, typename Second>
            struct tuples_has_no_same_non_const_type {
                static const bool value = false;
            };

            template <typename... Args1, typename Args2First, typename... Args2>
            struct tuples_has_no_same_non_const_type<
                std::tuple<Args1...>, std::tuple<Args2First, Args2...>> {
                bool value() {
                    if constexpr (sizeof...(Args1) == 0) {
                        return true;
                    } else if ((!std::is_const_v<Args2First>)&&type_in_group<
                                   Args2First, Args1...>()) {
                        return false;
                    } else {
                        if constexpr (sizeof...(Args2) > 0) {
                            return tuples_has_no_same_non_const_type<
                                       std::tuple<Args1...>,
                                       std::tuple<Args2...>>{}
                                .value();
                        } else {
                            return true;
                        }
                    }
                }
            };

            bool value() {
                if constexpr ((!std::is_same_v<Qu1, entt::entity>)&&(
                                  !std::is_same_v<Qu2, entt::entity>)) {
                    // if Qus1 and Qus2 are all const return false
                    if (args_all_const<Qu1, Qus1..., Qu2, Qus2...>()) {
                        return false;
                    }

                    // if Exs1 contains any non-const Type in Qus2 return
                    // false
                    if (first_tuple_contains_any_type_in_second_tuple<
                            std::tuple<Exs2...>, std::tuple<Qu1, Qus1...>>{}
                            .value()) {
                        return false;
                    }

                    // if Exs2 contains any non-const Type in Qus1 return
                    // false
                    if (first_tuple_contains_any_type_in_second_tuple<
                            std::tuple<Exs1...>, std::tuple<Qu2, Qus2...>>{}
                            .value()) {
                        return false;
                    }

                    // if Qus1 and Qus2 has no same non-const type return
                    // false
                    if (tuples_has_no_same_non_const_type<
                            std::tuple<Qu1, Qus1...>,
                            std::tuple<Qu2, Qus2...>>{}
                            .value()) {
                        return false;
                    }
                } else if constexpr (std::is_same_v<Qu1, entt::entity>) {
                    // if Qus1 and Qus2 are all const return false
                    if (args_all_const<Qus1..., Qu2, Qus2...>()) {
                        return false;
                    }

                    // if Exs1 contains any non-const Type in Qus2 return
                    // false
                    if (first_tuple_contains_any_type_in_second_tuple<
                            std::tuple<Exs2...>, std::tuple<Qus1...>>{}
                            .value()) {
                        return false;
                    }

                    // if Exs2 contains any non-const Type in Qus1 return
                    // false
                    if (first_tuple_contains_any_type_in_second_tuple<
                            std::tuple<Exs1...>, std::tuple<Qu2, Qus2...>>{}
                            .value()) {
                        return false;
                    }

                    // if Qus1 and Qus2 has no same non-const type return
                    // false
                    if (tuples_has_no_same_non_const_type<
                            std::tuple<Qus1...>, std::tuple<Qu2, Qus2...>>{}
                            .value()) {
                        return false;
                    }
                } else if constexpr (std::is_same_v<Qu2, entt::entity>) {
                    // if Qus1 and Qus2 are all const return false
                    if (args_all_const<Qu1, Qus1..., Qus2...>()) {
                        return false;
                    }

                    // if Exs1 contains any non-const Type in Qus2 return
                    // false
                    if (first_tuple_contains_any_type_in_second_tuple<
                            std::tuple<Exs2...>, std::tuple<Qu1, Qus1...>>{}
                            .value()) {
                        return false;
                    }

                    // if Exs2 contains any non-const Type in Qus1 return
                    // false
                    if (first_tuple_contains_any_type_in_second_tuple<
                            std::tuple<Exs1...>, std::tuple<Qus2...>>{}
                            .value()) {
                        return false;
                    }

                    // if Qus1 and Qus2 has no same non-const type return
                    // false
                    if (tuples_has_no_same_non_const_type<
                            std::tuple<Qu1, Qus1...>, std::tuple<Qus2...>>{}
                            .value()) {
                        return false;
                    }
                }

                return true;
            }
        };
    }  // namespace entity
}  // namespace pixel_engine