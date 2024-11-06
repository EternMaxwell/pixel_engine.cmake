#pragma once

#include "app/app.h"
#include "app/runner.h"
#include "app/runner_tools.h"
#include "app/stage_runner.h"
#include "app/subapp.h"
#include "app/substage_runner.h"
#include "app/system.h"
#include "app/tools.h"
#include "app/world.h"

namespace pixel_engine::prelude {
using namespace pixel_engine;

// ENTITY PART
using App      = app::App;
using Plugin   = app::Plugin;
using Entity   = app::Entity;
using Bundle   = internal_components::Bundle;
using Parent   = internal_components::Parent;
using Children = internal_components::Children;

// EVENTS
using AppExit = app::AppExit;

// SYSTEM PARA PART
template <typename T>
using Res = app::Res<T>;
template <typename T>
using ResMut  = app::ResMut<T>;
using Command = app::Command;
template <typename T>
using EventReader = app::EventReader<T>;
template <typename T>
using EventWriter = app::EventWriter<T>;
template <typename T>
using State = app::State<T>;
template <typename T>
using NextState = app::NextState<T>;
template <typename... Args>
using Get = app::Get<Args...>;
template <typename... Args>
using With = app::With<Args...>;
template <typename... Args>
using Without = app::Without<Args...>;
template <typename G, typename I = With<>, typename E = Without<>>
using Query = app::Query<G, I, E>;
template <typename G, typename I = With<>, typename E = Without<>>
using Extract = app::Extract<G, I, E>;
template <typename T>
using Local = app::Local<T>;
}  // namespace pixel_engine::prelude
