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
        template <typename... T>
        struct Get {};

        template <typename... T>
        struct With {};

        template <typename... T>
        struct Without {};

        namespace query {
            template <typename Get, typename With, typename Without>
            class Query {};

            template <typename... Qus, typename... Ins, typename... Exs>
            class Query<Get<Qus...>, With<Ins...>, Without<Exs...>> {
               private:
                entt::registry& registry;
                decltype(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{}).each()) m_iter;

               public:
                Query(entt::registry& registry) : registry(registry) {
                    m_iter = registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{}).each();
                }

                class iterator : public std::iterator<std::input_iterator_tag, std::tuple<Qus...>> {
                   private:
                    decltype(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{}).each()) m_full;
                    decltype(m_full.begin()) m_iter;

                    template <typename T, typename... Args>
                    std::tuple<Args...> remove_first(std::tuple<T, Args...> tuple) {
                        return std::apply([](T t, Args... args) { return std::tuple<Args...>(args...); }, tuple);
                    }

                   public:
                    iterator(decltype(m_full) full) : m_full(full), m_iter(full.begin()) {}

                    iterator(decltype(m_full) full, decltype(m_full.begin()) iter) : m_full(full), m_iter(iter) {}
                    iterator& operator++() {
                        m_iter++;
                        return *this;
                    }
                    iterator operator++(int) {
                        iterator tmp = *this;
                        ++(*this);
                        return tmp;
                    }
                    bool operator==(const iterator& rhs) const { return m_iter == rhs.m_iter; }
                    bool operator!=(const iterator& rhs) const { return m_iter != rhs.m_iter; }
                    auto operator*() {
                        // remove the first element of the tuple and return the
                        // rest
                        return std::tuple<Qus&...>(std::get<Qus&>(*m_iter)...);
                    }
                    auto begin() { return iterator(m_full, m_full.begin()); }

                    auto end() { return iterator(m_full, m_full.end()); }
                };

                /*! @brief Get the iterator for the query.
                 * @return The iterator for the query.
                 */
                auto iter() { return iterator(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{}).each()); }

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

            template <typename... Qus, typename... Ins, typename... Exs>
            class Query<Get<entt::entity, Qus...>, With<Ins...>, Without<Exs...>> {
               private:
                entt::registry& registry;
                decltype(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{}).each()) m_iter;

               public:
                Query(entt::registry& registry) : registry(registry) {
                    m_iter = registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{}).each();
                }

                class iterator : public std::iterator<std::input_iterator_tag, std::tuple<Qus...>> {
                   private:
                    decltype(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{}).each()) m_full;
                    decltype(m_full.begin()) m_iter;

                    template <typename T, typename... Args>
                    std::tuple<Args...> remove_first(std::tuple<T, Args...> tuple) {
                        return std::apply([](T t, Args... args) { return std::tuple<Args...>(args...); }, tuple);
                    }

                   public:
                    iterator(decltype(m_full) full) : m_full(full), m_iter(full.begin()) {}

                    iterator(decltype(m_full) full, decltype(m_full.begin()) iter) : m_full(full), m_iter(iter) {}
                    iterator& operator++() {
                        m_iter++;
                        return *this;
                    }
                    iterator operator++(int) {
                        iterator tmp = *this;
                        ++(*this);
                        return tmp;
                    }
                    bool operator==(const iterator& rhs) const { return m_iter == rhs.m_iter; }
                    bool operator!=(const iterator& rhs) const { return m_iter != rhs.m_iter; }
                    auto operator*() {
                        // remove the first element of the tuple and return the
                        // rest
                        return std::tuple<entt::entity, Qus&...>(
                            std::get<entt::entity>(*m_iter), std::get<Qus&>(*m_iter)...);
                    }
                    auto begin() { return iterator(m_full, m_full.begin()); }

                    auto end() { return iterator(m_full, m_full.end()); }
                };

                /*! @brief Get the iterator for the query.
                 * @return The iterator for the query.
                 */
                auto iter() { return iterator(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{}).each()); }

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
        }  // namespace query

        template <typename Get, typename With, typename Without>
        using Query = query::Query<Get, With, Without>;
    }  // namespace entity
}  // namespace pixel_engine