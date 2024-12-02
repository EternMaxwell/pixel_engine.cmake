#pragma once

#include <pixel_engine/common.h>
// #define IMGUI_API EPIX_API
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <pixel_engine/render_vk.h>
#include <pixel_engine/window.h>

#include "resources.h"

namespace pixel_engine::imgui::systems {
using namespace pixel_engine::prelude;
using namespace pixel_engine::render_vk::components;
using namespace pixel_engine::window::components;
using namespace pixel_engine::imgui;
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
}  // namespace pixel_engine::imgui::systems