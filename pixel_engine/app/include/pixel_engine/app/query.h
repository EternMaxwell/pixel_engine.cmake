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
class QueryBase {};

template <typename... Qus, typename... Ins, typename... Exs>
class QueryBase<Get<Qus...>, With<Ins...>, Without<Exs...>> {
   private:
    entt::registry& registry;
    decltype(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{})) m_view;

   public:
    QueryBase(entt::registry& registry) : registry(registry) {
        m_view = registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{});
    }

    class iterator
        : public std::iterator<std::input_iterator_tag, std::tuple<Qus...>> {
       private:
        decltype(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{}).each()
        ) m_full;
        decltype(m_full.begin()) m_iter;

        template <typename T, typename... Args>
        std::tuple<Args...> remove_first(std::tuple<T, Args...> tuple) {
            return std::apply(
                [](T t, Args... args) { return std::tuple<Args...>(args...); },
                tuple
            );
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

    void for_each(std::function<void(Qus&...)> func) { m_view.each(func); }
};

template <typename... Qus, typename... Ins, typename... Exs>
class QueryBase<Get<entt::entity, Qus...>, With<Ins...>, Without<Exs...>> {
   private:
    entt::registry& registry;
    decltype(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{})) m_view;

   public:
    QueryBase(entt::registry& registry) : registry(registry) {
        m_view = registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{});
    }

    class iterator
        : public std::iterator<std::input_iterator_tag, std::tuple<Qus...>> {
       private:
        decltype(registry.view<Qus..., Ins...>(entt::exclude_t<Exs...>{}).each()
        ) m_full;
        decltype(m_full.begin()) m_iter;

        template <typename T, typename... Args>
        std::tuple<Args...> remove_first(std::tuple<T, Args...> tuple) {
            return std::apply(
                [](T t, Args... args) { return std::tuple<Args...>(args...); },
                tuple
            );
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
                std::get<entt::entity>(*m_iter), std::get<Qus&>(*m_iter)...
            );
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

    void for_each(std::function<void(entt::entity, Qus&...)> func) {
        m_view.each(func);
    }
};

template <typename G, typename W, typename WO>
struct Extract {};

template <typename... Gets, typename... Withs, typename... Withouts>
struct Extract<Get<Gets...>, With<Withs...>, Without<Withouts...>> {
    using type = QueryBase<
        Get<std::add_const_t<Gets>...>,
        With<std::add_const_t<Withs>...>,
        Without<std::add_const_t<Withouts>...>>;

   private:
    type query;

   public:
    Extract(entt::registry& registry) : query(registry) {}
    Extract(type query) : query(query) {}
    Extract(type&& query) : query(query) {}
    auto iter() { return query.iter(); }
    auto single() { return query.single(); }
    void for_each(std::function<void(typename type::iterator::value_type)> func
    ) {
        query.for_each(func);
    }
    auto get(entt::entity id) { return query.get(id); }
    bool contains(entt::entity id) { return query.contains(id); }
};

template <typename... Gets, typename... Withs, typename... Withouts>
struct Extract<
    Get<entt::entity, Gets...>,
    With<Withs...>,
    Without<Withouts...>> {
    using type = QueryBase<
        Get<entt::entity, std::add_const_t<Gets>...>,
        With<std::add_const_t<Withs>...>,
        Without<std::add_const_t<Withouts>...>>;

   private:
    type query;

   public:
    Extract(entt::registry& registry) : query(registry) {}
    Extract(type query) : query(query) {}
    Extract(type&& query) : query(query) {}
    auto iter() { return query.iter(); }
    auto single() { return query.single(); }
    void for_each(std::function<void(typename type::iterator::value_type)> func
    ) {
        query.for_each(func);
    }
    auto get(entt::entity id) { return query.get(id); }
    bool contains(entt::entity id) { return query.contains(id); }
};

template <typename G, typename W, typename WO>
struct Query {
    using type = QueryBase<G, W, WO>;

   private:
    type query;

   public:
    Query(entt::registry& registry) : query(registry) {}
    Query(type query) : query(query) {}
    Query(type&& query) : query(query) {}
    auto iter() { return query.iter(); }
    auto single() { return query.single(); }
    void for_each(std::function<void(typename type::iterator::value_type)> func
    ) {
        query.for_each(func);
    }
    auto get(entt::entity id) { return query.get(id); }
    bool contains(entt::entity id) { return query.contains(id); }
};
}  // namespace app
}  // namespace pixel_engine
