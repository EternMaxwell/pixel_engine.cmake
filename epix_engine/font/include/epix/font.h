#pragma once

#include "font/components.h"
#include "font/resources.h"
#include "font/systems.h"

namespace epix {
namespace font {
using namespace prelude;
struct FontPlugin : Plugin {
    uint32_t canvas_width  = 4096;
    uint32_t canvas_height = 1024;
    EPIX_API void build(App& app) override;
};
}  // namespace font
}  // namespace epix