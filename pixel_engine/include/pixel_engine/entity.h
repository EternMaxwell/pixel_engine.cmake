#pragma once

#include "pixel_engine/entity/entity.h"

#pragma once

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace prelude {
        // app
        using App = entity::App;

        // events
        using AppExit = entity::AppExit;

        // system node
        using SystemNode = std::shared_ptr<entity::SystemNode>;

        // bundle
        using Bundle = entity::Bundle;

        // entity
        using Entity = entt::entity;

        // query types
        template <typename... T>
        using Get = entity::Get<T...>;
        template <typename... T>
        using With = entity::With<T...>;
        template <typename... T>
        using Without = entity::Without<T...>;

        // system arguments
        using Command = entity::Command;
        template <typename T>
        using Resource = entity::Resource<T>;
        template <typename In, typename Ws = With<>, typename Ex = Without<>>
        using Query = entity::Query<In, Ws, Ex>;
        template <typename T>
        using EventReader = entity::EventReader<T>;
        template <typename T>
        using EventWriter = entity::EventWriter<T>;
        template <typename T>
        using State = entity::State<T>;
        template <typename T>
        using NextState = entity::NextState<T>;

        // sequential run
        using after = entity::after;
        using before = entity::before;
        template <typename... Args>
        using in_set = entity::in_set<Args...>;

        // condition run
        template <typename... Args>
        using run_if = entity::run_if<Args...>;
        template <typename T>
        using in_state = entity::in_state<T>;

        // schedulers
        using Startup = entity::Startup;
        template <typename T>
        using OnEnter = entity::OnEnter<T>;
        template <typename T>
        using OnExit = entity::OnExit<T>;
        using PreUpdate = entity::PreUpdate;
        using Update = entity::Update;
        using PostUpdate = entity::PostUpdate;
        using PreRender = entity::PreRender;
        using Render = entity::Render;
        using PostRender = entity::PostRender;

        // plugins
        using Plugin = entity::Plugin;
        using LoopPlugin = entity::LoopPlugin;
    }  // namespace prelude
}  // namespace pixel_engine