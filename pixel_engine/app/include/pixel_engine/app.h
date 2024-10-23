#pragma once

#include "app/app.h"

namespace pixel_engine {
namespace prelude {
using App = app::App;
using AppExit = app::AppExit;

using Bundle = app::Bundle;
using Entity = entt::entity;

template <typename T>
using Handle = app::Handle<T>;

// query types
template <typename... T>
using Get = app::Get<T...>;
template <typename... T>
using With = app::With<T...>;
template <typename... T>
using Without = app::Without<T...>;

using Command = app::Command;
template <typename T>
using Resource = app::Resource<T>;
template <typename In, typename Ws = With<>, typename Ex = Without<>>
using Query = app::Query<In, Ws, Ex>;
template <typename In, typename Ws = With<>, typename Ex = Without<>>
using Extract = app::Extract<In, Ws, Ex>;
template <typename T>
using EventReader = app::EventReader<T>;
template <typename T>
using EventWriter = app::EventWriter<T>;
template <typename T>
using State = app::State<T>;
template <typename T>
using NextState = app::NextState<T>;

using Plugin = app::Plugin;

template <typename T>
std::shared_ptr<app::BasicSystem<bool>> in_state(const T&& t) {
    return std::make_shared<app::Condition<Resource<State<T>>>>(
        [&](Resource<State<T>> cur) { return cur->is_state(t); }
    );
}
}  // namespace prelude
}  // namespace pixel_engine