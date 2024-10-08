﻿#pragma once

#include <glad/glad.h>

#include "components.h"
#include "pixel_engine/entity.h"
#include "pixel_engine/window.h"

namespace pixel_engine {
namespace render_gl {
namespace systems {
using namespace prelude;
using namespace components;
using namespace window::components;

void clear_color(
    Query<Get<WindowHandle>, With<WindowCreated>, Without<>> query);

void update_viewport(
    Query<Get<WindowHandle, WindowSize>, With<WindowCreated>, Without<>> query);

void context_creation(
    Query<Get<WindowHandle>, With<WindowCreated>, Without<>> query);

void complete_pipeline(
    Command command,
    Query<
        Get<entt::entity, ProgramShaderAttachments, VertexAttribs>,
        With<PipelineCreation>, Without<>>
        query);
}  // namespace systems
}  // namespace render_gl
}  // namespace pixel_engine