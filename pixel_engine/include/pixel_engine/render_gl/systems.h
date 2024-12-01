#pragma once

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

EPIX_API void clear_color(Query<Get<Window>> query);

EPIX_API void update_viewport(Query<Get<Window>> query);

EPIX_API void context_creation(Query<Get<Window>> query);

EPIX_API void swap_buffers(Query<Get<Window>> query);

EPIX_API void complete_pipeline(
    Command command,
    Query<
        Get<Entity, ProgramShaderAttachments, VertexAttribs>,
        With<PipelineCreation>,
        Without<>> query
);
}  // namespace systems
}  // namespace render_gl
}  // namespace pixel_engine