#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include "components.h"
#include "events.h"

namespace pixel_engine {
namespace window {

class WindowPlugin;
namespace systems {
using namespace components;
using namespace events;
using namespace prelude;

void init_glfw();
void insert_primary_window(
    Command command, Resource<window::WindowPlugin> window_plugin);
void create_window(
    Command command, Query<
                         Get<entt::entity, WindowHandle, const WindowSize,
                             const WindowTitle, const WindowHints>,
                         With<>, Without<WindowCreated>>
                         query);
void update_window_size(
    Query<Get<WindowHandle, WindowSize>, With<WindowCreated>, Without<>> query);
void update_window_pos(
    Query<Get<WindowHandle, WindowPos>, With<WindowCreated>, Without<>> query);
void primary_window_close(
    Command command,
    Query<
        Get<entt::entity, WindowHandle>, With<WindowCreated, PrimaryWindow>,
        Without<>>
        query,
    EventWriter<AnyWindowClose> any_close_event);
void window_close(
    Command command,
    Query<
        Get<entt::entity, WindowHandle>, With<WindowCreated>,
        Without<PrimaryWindow>>
        query,
    EventWriter<AnyWindowClose> any_close_event);
void swap_buffers(
    Query<Get<const WindowHandle>, With<WindowCreated>, Without<>> query);
void no_window_exists(
    Query<Get<const WindowHandle>, With<>, Without<>> query,
    EventWriter<NoWindowExists> no_window_event);
void poll_events();
void exit_on_no_window(
    EventReader<NoWindowExists> no_window_event,
    EventWriter<AppExit> exit_event);
}  // namespace systems
}  // namespace window
}  // namespace pixel_engine