#pragma once

#include <pixel_engine/app.h>
#include <pixel_engine/window.h>
#include <spdlog/spdlog.h>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include "components.h"
#include "resources.h"

namespace pixel_engine {
namespace render_vk {
namespace systems {
using namespace pixel_engine::prelude;
using namespace components;
using namespace window::components;

void create_device(Command cmd);
}  // namespace systems
}  // namespace render_vk
}  // namespace pixel_engine