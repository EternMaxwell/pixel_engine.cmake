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
template <typename T>
using Handle = internal_components::Handle<T>;

// SYSTEM PARA PART
template <typename T>
using Res = app::Res<T>;
template <typename T>
using ResMut = app::ResMut<T>;
using Command = app::Command;
template <typename T>
using EventReader = app::EventReader<T>;
template <typename T>
using EventWriter = app::EventWriter<T>;
template <typename T>
using State = app::State<T>;
template <typename T>
using NextState = app::NextState<T>;
template <typename G, typename I, typename E>
using Query = app::Query<G, I, E>;
template <typename G, typename I, typename E>
using Extract = app::Extract<G, I, E>;
template <typename T>
using Local = app::Local<T>;
}  // namespace pixel_engine::prelude
