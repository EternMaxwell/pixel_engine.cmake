#pragma once

#include "pixel_engine/entity.h"
#include "pixel_engine/plugins/asset_server_gl.h"
#include "pixel_engine/plugins/render_gl.h"
#include "pixel_engine/plugins/window_gl.h"

namespace pixel_engine {
    namespace prelude {
        namespace winGL = plugins::window_gl;
        namespace assetGL = plugins::asset_server_gl;
        namespace renderGL = plugins::render_gl;

        // app
        using App = entity::App;

        // system node
        using SystemNode = std::shared_ptr<entity::SystemNode>;

        // query types
        template <typename... T>
        using Get = entity::Get<T...>;
        template <typename... T>
        using Without = entity::Without<T...>;

        // system arguments
        using Command = entity::Command;
        template <typename T>
        using Resource = entity::Resource<T>;
        template <typename In, typename Ex>
        using Query = entity::Query<In, Ex>;
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
        using WindowGLPlugin = plugins::WindowGLPlugin;
    }  // namespace prelude
}  // namespace pixel_engine