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

        using App = entity::App;
        using Plugin = entity::Plugin;
        using LoopPlugin = entity::LoopPlugin;
        using WindowGLPlugin = plugins::WindowGLPlugin;
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

        using after = entity::after;
        using before = entity::before;

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
    }  // namespace prelude
}  // namespace pixel_engine