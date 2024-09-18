#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1002000
#endif
#include <vk_mem_alloc.h>

#include "vulkan/instance.h"

namespace pixel_engine {
namespace render_vk {
namespace components {}
}  // namespace render_vk
}  // namespace pixel_engine