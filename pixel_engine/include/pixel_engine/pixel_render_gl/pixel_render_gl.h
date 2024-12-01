#pragma once

#include "components.h"
#include "systems.h"

namespace pixel_engine {
namespace pixel_render_gl {
using namespace prelude;

struct PixelRenderGLPlugin : public Plugin {
   public:
    EPIX_API void build(App& app) override;
};
}  // namespace pixel_render_gl
}  // namespace pixel_engine