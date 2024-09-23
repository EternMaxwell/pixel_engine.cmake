#pragma once

#include <unordered_map>

#include "vulkan/device.h"
#include "vulkan_headers.h"

namespace pixel_engine {
namespace render_vk {
namespace components {
using namespace vulkan;
struct RenderContext {
    Instance instance;
    Device device;
    SwapChain swap_chain;
};
}  // namespace components
}  // namespace render_vk
}  // namespace pixel_engine