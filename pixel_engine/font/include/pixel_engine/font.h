#pragma once

#include "font/components.h"
#include "font/resources.h"
#include "font/systems.h"

namespace pixel_engine {
namespace font {
using namespace prelude;
struct FontPlugin : Plugin {
    uint32_t canvas_width  = 4096;
    uint32_t canvas_height = 1024;
    void build(App& app) override;
};
}  // namespace font
}  // namespace pixel_engine