#pragma once

#include <entt/entity/registry.hpp>
#include <tuple>

namespace pixel_engine {
namespace app {
struct Entity {
    entt::entity id = entt::null;
    Entity operator=(entt::entity id) {
        this->id = id;
        return *this;
    }
    operator entt::entity() { return id; }
    operator bool() { return id != entt::null; }
    bool operator!() { return id == entt::null; }
    bool operator==(const Entity& other) { return id == other.id; }
    bool operator!=(const Entity& other) { return id != other.id; }
    bool operator==(const entt::entity& other) { return id == other; }
    bool operator!=(const entt::entity& other) { return id != other; }
};
struct App;
struct SubApp;
}  // namespace app
namespace internal_components {
using Entity = app::Entity;
struct Bundle {};
struct Parent {
    Entity id;
};
template <typename T>
struct Handle {
    Entity id;
    void operator=(entt::entity id) { this->id = id; }
    void operator=(Entity id) { this->id = id; }
    operator entt::entity() { return id; }
    operator Entity() { return id; }
    operator bool() { return id.operator bool(); }
    bool operator!() { return !id; }
    bool operator==(const Handle<T>& other) { return id == other.id; }
    bool operator!=(const Handle<T>& other) { return id != other.id; }
    bool operator==(const entt::entity& other) { return id == other; }
    bool operator!=(const entt::entity& other) { return id != other; }
    bool operator==(const Entity& other) { return id == other.id; }
    bool operator!=(const Entity& other) { return id != other.id; }
};
template <typename T>
struct NextState;
template <typename T>
struct State {
    friend class app::App;
    friend class app::SubApp;

   protected:
    T m_state;
    bool just_created = true;

   public:
    State() : m_state() {}
    State(const T& state) : m_state(state) {}
    bool is_just_created() { return just_created; }

    bool is_state(const T& state) { return m_state == state; }
    bool is_state(const NextState<T>& state) {
        return m_state == state.m_state;
    }
};
template <typename T>
struct NextState : public State<T> {
   public:
    NextState() : State<T>() {}
    NextState(const T& state) : State<T>(state) {}

    void set_state(const T& state) { State<T>::m_state = state; }
};
}  // namespace internal_components
namespace app_tools {
using Bundle = internal_components::Bundle;
template <template <typename...> typename Template, typename T>
struct is_template_of : std::false_type {};

template <template <typename...> typename Template, typename... Args>
struct is_template_of<Template, Template<Args...>> : std::true_type {};

template <typename T, typename = void, typename = void>
struct is_bundle : std::false_type {};

template <typename T>
struct is_bundle<T, std::void_t<decltype(std::declval<T>().unpack())>>
    : std::true_type {};

template <typename... Args>
void registry_emplace_tuple(
    entt::registry* registry, entt::entity entity, std::tuple<Args...>&& tuple
);

template <typename T>
void registry_emplace_single(
    entt::registry* registry, entt::entity entity, T&& arg
) {
    if constexpr (is_bundle<std::remove_reference_t<T>>::value &&
                  std::is_base_of_v<Bundle, std::remove_reference_t<T>>) {
        registry_emplace_tuple(registry, entity, std::forward<T>(arg).unpack());
    } else if constexpr (!std::is_same_v<Bundle, std::remove_reference_t<T>>) {
        registry->emplace<std::remove_reference_t<T>>(entity, std::move(arg));
    }
}

template <typename... Args>
void registry_emplace(
    entt::registry* registry, entt::entity entity, Args&&... args
) {
    auto&& arr = {(
        registry_emplace_single(registry, entity, std::forward<Args>(args)), 0
    )...};
}

template <typename... Args, size_t... I>
void registry_emplace_tuple(
    entt::registry* registry,
    entt::entity entity,
    std::tuple<Args...>&& tuple,
    std::index_sequence<I...> _
) {
    registry_emplace(
        registry, entity,
        std::forward<decltype(std::get<I>(tuple))>(std::get<I>(tuple))...
    );
}

template <typename... Args>
void registry_emplace_tuple(
    entt::registry* registry, entt::entity entity, std::tuple<Args...>&& tuple
) {
    registry_emplace_tuple(
        registry, entity, std::forward<decltype(tuple)>(tuple),
        std::make_index_sequence<sizeof...(Args)>()
    );
}

template <typename... Args>
void registry_erase(entt::registry* registry, entt::entity entity) {
    auto&& arr = {(registry->remove<Args>(entity), 0)...};
}

template <typename T, typename Tuple>
struct tuple_contain;

template <typename T>
struct tuple_contain<T, std::tuple<>> : std::false_type {};

template <typename T, typename U, typename... Ts>
struct tuple_contain<T, std::tuple<U, Ts...>>
    : tuple_contain<T, std::tuple<Ts...>> {};

template <typename T, typename... Ts>
struct tuple_contain<T, std::tuple<T, Ts...>> : std::true_type {};

template <template <typename...> typename T, typename Tuple>
struct tuple_contain_template;

template <template <typename...> typename T>
struct tuple_contain_template<T, std::tuple<>> : std::false_type {};

template <template <typename...> typename T, typename U, typename... Ts>
struct tuple_contain_template<T, std::tuple<U, Ts...>>
    : tuple_contain_template<T, std::tuple<Ts...>> {};

template <template <typename...> typename T, typename... Ts, typename... Temps>
struct tuple_contain_template<T, std::tuple<T<Temps...>, Ts...>>
    : std::true_type {};

template <template <typename...> typename T, typename Tuple>
struct tuple_template_index {};

template <template <typename...> typename T, typename U, typename... Args>
struct tuple_template_index<T, std::tuple<U, Args...>> {
    static constexpr int index() {
        if constexpr (is_template_of<T, U>::value) {
            return 0;
        } else {
            return 1 + tuple_template_index<T, std::tuple<Args...>>::index();
        }
    }
};

template <typename T, typename... Args>
T tuple_get(std::tuple<Args...> tuple) {
    if constexpr (tuple_contain<T, std::tuple<Args...>>::value) {
        return std::get<T>(tuple);
    } else {
        return T();
    }
}

template <template <typename...> typename T, typename... Args>
constexpr auto tuple_get_template(std::tuple<Args...> tuple) {
    if constexpr (tuple_contain_template<T, std::tuple<Args...>>::value) {
        return std::get<tuple_template_index<T, std::tuple<Args...>>::index()>(
            tuple
        );
    } else {
        return T();
    }
}

template <typename T>
struct t_weak_ptr : std::weak_ptr<T> {
    t_weak_ptr(std::shared_ptr<T> ptr) : std::weak_ptr<T>(ptr) {}
    t_weak_ptr(std::weak_ptr<T> ptr) : std::weak_ptr<T>(ptr) {}

    T* get_p() { return std::weak_ptr<T>::get(); }
};

template <typename T>
struct std::hash<std::weak_ptr<T>> {
    size_t operator()(const std::weak_ptr<T>& ptr) const {
        pixel_engine::app_tools::t_weak_ptr<T> tptr(ptr);
        return std::hash<T*>()(tptr.get_p());
    }
};

template <typename T>
struct std::equal_to<std::weak_ptr<T>> {
    bool operator()(const std::weak_ptr<T>& a, const std::weak_ptr<T>& b)
        const {
        pixel_engine::app_tools::t_weak_ptr<T> aptr(a);
        pixel_engine::app_tools::t_weak_ptr<T> bptr(b);
        return aptr.get_p() == bptr.get_p();
    }
};

template <typename T>
struct std::hash<pixel_engine::internal_components::Handle<T>> {
    size_t operator()(const pixel_engine::internal_components::Handle<T>& handle
    ) const {
        return std::hash<entt::entity>()(handle.id);
    }
};

template <typename T>
struct std::equal_to<pixel_engine::internal_components::Handle<T>> {
    bool operator()(
        const pixel_engine::internal_components::Handle<T>& a,
        const pixel_engine::internal_components::Handle<T>& b
    ) const {
        return a.id == b.id;
    }
};
}  // namespace app_tools
}  // namespace pixel_engine