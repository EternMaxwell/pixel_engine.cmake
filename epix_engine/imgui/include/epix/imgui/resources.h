#pragma once

#include <epix/common.h>
// #define IMGUI_API EPIX_API
#include <epix/render_vk.h>
#include <epix/window.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>


namespace epix::imgui {
using namespace epix::prelude;
using namespace epix::render_vk::components;
using namespace epix::window::components;
struct ImGuiContext {
    DescriptorPool descriptor_pool;
    RenderPass render_pass;
    CommandBuffer command_buffer;
    Fence fence;
    Framebuffer framebuffer;
    ::ImGuiContext* context;

    EPIX_API ::ImGuiContext* current_context();
};
}  // namespace epix::imgui