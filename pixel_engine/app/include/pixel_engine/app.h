#pragma once

#include "app/app.h"

namespace pixel_engine {
namespace prelude {
using App = app::App;
using AppExit = app::AppExit;

using Bundle = app::Bundle;
using Entity = entt::entity;

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
template <typename T>
using EventReader = app::EventReader<T>;
template <typename T>
using EventWriter = app::EventWriter<T>;
template <typename T>
using State = app::State<T>;
template <typename T>
using NextState = app::NextState<T>;

// sequential run
using after = app::after;
using before = app::before;
using in_set = app::in_set;

using Plugin = app::Plugin;

app::Schedule PreStartup = app::Schedule(app::StartStage::PreStartup);
app::Schedule Startup = app::Schedule(app::StartStage::Startup);
app::Schedule PostStartup = app::Schedule(app::StartStage::PostStartup);
app::Schedule First = app::Schedule(app::LoopStage::First);
app::Schedule PreUpdate = app::Schedule(app::LoopStage::PreUpdate);
app::Schedule Update = app::Schedule(app::LoopStage::Update);
app::Schedule PostUpdate = app::Schedule(app::LoopStage::PostUpdate);
app::Schedule Last = app::Schedule(app::LoopStage::Last);
app::Schedule Prepare = app::Schedule(app::RenderStage::Prepare);
app::Schedule PreRender = app::Schedule(app::RenderStage::PreRender);
app::Schedule Render = app::Schedule(app::RenderStage::Render);
app::Schedule PostRender = app::Schedule(app::RenderStage::PostRender);
app::Schedule PreShutdown = app::Schedule(app::ExitStage::PreShutdown);
app::Schedule Shutdown = app::Schedule(app::ExitStage::Shutdown);
app::Schedule PostShutdown = app::Schedule(app::ExitStage::PostShutdown);
template <typename T>
app::Schedule OnEnter(T t) {
    return app::Schedule(
        app::StateTransit,
        [=](Resource<State<T>> cur, Resource<NextState<T>> next) {
            if (cur->is_just_created() && cur->is_state(t)) return true;
            if (!cur->is_state(t) && next->is_state(t)) return true;
            return false;
        }
    );
}
template <typename T>
app::Schedule OnExit(T t) {
    return app::Schedule(
        app::StateTransit,
        [=](Resource<State<T>> cur, Resource<NextState<T>> next) {
            if (cur->is_state(t) && !next->is_state(t)) return true;
            return false;
        }
    );
}
template <typename T>
app::Schedule OnChange() {
    return app::Schedule(
        app::StateTransit,
        [=](Resource<State<T>> cur, Resource<NextState<T>> next) {
            if (cur->is_state(next)) return true;
            return false;
        }
    );
}
template <typename T>
std::shared_ptr<app::BasicSystem<bool>> in_state(const T&& t) {
    return std::make_shared<app::Condition<Resource<State<T>>>>(
        [&](Resource<State<T>> cur) { return cur->is_state(t); }
    );
}
}  // namespace prelude
}  // namespace pixel_engine