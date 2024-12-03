#pragma once

#include <epix/common.h>
// #define IMGUI_API EPIX_API
#include <epix/render_vk.h>
#include <epix/window.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "resources.h"


namespace epix::imgui::systems {
using namespace epix::prelude;
using namespace epix::render_vk::components;
using namespace epix::window::components;
using namespace epix::imgui;
EPIX_API void insert_imgui_ctx(Command cmd);
EPIX_API void init_imgui(
    Query<
        Get<Instance, PhysicalDevice, Device, Queue, CommandPool>,
        With<RenderContext>> context_query,
    Query<Get<Window>, With<PrimaryWindow>> window_query,
    ResMut<ImGuiContext> imgui_context
);
EPIX_API void deinit_imgui(
    Query<Get<Device, CommandPool>, With<RenderContext>> context_query,
    ResMut<ImGuiContext> imgui_context
);
EPIX_API void begin_imgui(
    ResMut<ImGuiContext> ctx,
    Query<Get<Device, Queue, Swapchain>, With<RenderContext>> query
);
EPIX_API void end_imgui(
    ResMut<ImGuiContext> ctx,
    Query<Get<Device, Queue, Swapchain>, With<RenderContext>> query
);
}  // namespace epix::imgui::systems