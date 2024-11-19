#pragma once

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
};
}  // namespace pixel_engine::imgui

namespace pixel_engine::imgui::systems {
using namespace pixel_engine::prelude;
using namespace pixel_engine::render_vk::components;
using namespace pixel_engine::window::components;
using namespace pixel_engine::imgui;
void insert_imgui_ctx(Command cmd);
void init_imgui(
    Query<
        Get<Instance, PhysicalDevice, Device, Queue, CommandPool>,
        With<RenderContext>> context_query,
    Query<Get<Window>, With<PrimaryWindow>> window_query,
    ResMut<ImGuiContext> imgui_context
);
void deinit_imgui(
    Query<Get<Device, CommandPool>, With<RenderContext>> context_query,
    ResMut<ImGuiContext> imgui_context
);
void begin_imgui(
    ResMut<ImGuiContext> ctx,
    Query<Get<Device, Queue, Swapchain>, With<RenderContext>> query
);
void end_imgui(
    ResMut<ImGuiContext> ctx,
    Query<Get<Device, Queue, Swapchain>, With<RenderContext>> query
);
}  // namespace pixel_engine::imgui::systems