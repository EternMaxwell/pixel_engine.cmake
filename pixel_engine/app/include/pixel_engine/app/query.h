#pragma once

#include <entt/entity/registry.hpp>
#include <optional>
#include <tuple>
#include <type_traits>

namespace pixel_engine {
namespace app {
template <typename... T>
struct Get {};

template <typename... T>
struct With {};

template <typename... T>
struct Without {};

template <typename Get, typename With, typename Without>
class Query {};

template <typename... Qus, typename... Ins, typename... Exs>
class Query<Get<Qus...>, With<Ins...>, Without<Exs...>> {
   private:
    entt::registry& registry;
    decltype(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{})) m_view;

   public:
    Query(entt::registry& registry) : registry(registry) {
        m_view = registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{});
    }

    class iterator
        : public std::iterator<std::input_iterator_tag, std::tuple<Qus...>> {
       private:
        decltype(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{})
                     .each()) m_full;
        decltype(m_full.begin()) m_iter;

        template <typename T, typename... Args>
        std::tuple<Args...> remove_first(std::tuple<T, Args...> tuple) {
            return std::apply(
                [](T t, Args... args) { return std::tuple<Args...>(args...); },
                tuple);
        }

       public:
        iterator(decltype(m_full) full) : m_full(full), m_iter(full.begin()) {}

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
    auto iter() { return iterator(m_view.each()); }

    std::tuple<Qus&...> get(entt::entity id) { return m_view.get<Qus...>(id); }

    bool contains(entt::entity id) { return m_view.contains(id); }

    /*! @brief Get the single entity and requaired components.
     * @return An optional of a single tuple of entity and requaired
     * components.
     */
    auto single() {
        // auto start = *(iter().begin());
        if (iter().begin() != iter().end()) {
            return std::optional(*(iter().begin()));
        } else {
            return std::optional<decltype(*(iter().begin()))>{};
        }
    }
};

template <typename... Qus, typename... Ins, typename... Exs>
class Query<Get<entt::entity, Qus...>, With<Ins...>, Without<Exs...>> {
   private:
    entt::registry& registry;
    decltype(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{})) m_view;

   public:
    Query(entt::registry& registry) : registry(registry) {
        m_view = registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{});
    }

    class iterator
        : public std::iterator<std::input_iterator_tag, std::tuple<Qus...>> {
       private:
        decltype(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{})
                     .each()) m_full;
        decltype(m_full.begin()) m_iter;

        template <typename T, typename... Args>
        std::tuple<Args...> remove_first(std::tuple<T, Args...> tuple) {
            return std::apply(
                [](T t, Args... args) { return std::tuple<Args...>(args...); },
                tuple);
        }

       public:
        iterator(decltype(m_full) full) : m_full(full), m_iter(full.begin()) {}

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
    auto iter() { return iterator(m_view.each()); }

    std::tuple<Qus&...> get(entt::entity id) { return m_view.get<Qus...>(id); }

    bool contains(entt::entity id) { return m_view.contains(id); }

    /*! @brief Get the single entity and requaired components.
     * @return An optional of a single tuple of entity and requaired
     * components.
     */
    auto single() {
        // auto start = *(iter().begin());
        if (iter().begin() != iter().end()) {
            return std::optional(*(iter().begin()));
        } else {
            return std::optional<decltype(*(iter().begin()))>{};
        }
    }
};
}  // namespace app
}  // namespace pixel_engine
