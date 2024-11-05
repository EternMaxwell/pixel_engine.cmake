#pragma once

#include <glad/glad.h>
#include <pixel_engine/app.h>
#include <pixel_engine/window.h>

namespace pixel_engine::render::ogl {
namespace systems {
using namespace prelude;

struct ContextCreated {};

void clear_color(Query<Get<window::Window>> query);
void update_viewport(Query<Get<window::Window>> query);
void context_creation(
    Command cmd,
    Query<Get<Entity, window::Window>, Without<ContextCreated>> query
);
void swap_buffers(Query<Get<window::Window>> query);
}  // namespace systems
}  // namespace pixel_engine::render::ogl