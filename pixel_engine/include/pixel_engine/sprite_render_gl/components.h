#pragma once

#include "pixel_engine/entity.h"
#include "pixel_engine/render_gl/components.h"
#include "pixel_engine/transform/components.h"

namespace pixel_engine {
namespace sprite_render_gl {
namespace components {
using namespace prelude;
using namespace transform;
using namespace render_gl::components;

struct Vertex {
    float position[3];
    float color[4];
    float tex_coords[2];
};

struct Sprite {
    Image texture;
    float center[2] = {0.5f, 0.5f};
    float size[2] = {1.0f, 1.0f};
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    void vertex_data(Vertex* vertices) const;
};

struct SpriteBundle {
    Bundle bundle;
    Sprite sprite;
    Transform transform;
};

struct SpritePipeline {};
}  // namespace components
}  // namespace sprite_render_gl
}  // namespace pixel_engine