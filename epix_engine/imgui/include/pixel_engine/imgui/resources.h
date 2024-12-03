#pragma once

#include <pixel_engine/common.h>
// #define IMGUI_API EPIX_API
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <pixel_engine/render_vk.h>
#include <pixel_engine/window.h>

namespace pixel_engine::imgui {
using namespace pixel_engine::prelude;
using namespace pixel_engine::render_vk::components;
using namespace pixel_engine::window::components;
struct ImGuiContext {
    DescriptorPool descriptor_pool;
    RenderPass render_pass;
    CommandBuffer command_buffer;
    Fence fence;
    Framebuffer framebuffer;
    ::ImGuiContext* context;

    EPIX_API ::ImGuiContext* current_context();
};
}  // namespace pixel_engine::imgui