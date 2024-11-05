#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#include <pixel_engine/app.h>

#include "components.h"
#include "events.h"

namespace pixel_engine {
namespace window {

class WindowPlugin;
namespace systems {
using namespace components;
using namespace events;
using namespace pixel_engine::prelude;

void init_glfw();
void insert_primary_window(
    Command command, ResMut<window::WindowPlugin> window_plugin
);
void create_window_start(
    Command command,
    Query<Get<Entity, const WindowDescription>, Without<Window>> desc_query
);
void create_window_update(
    Command command,
    Query<Get<Entity, const WindowDescription>, Without<Window>> desc_query
);
void update_window_cursor_pos(
    Query<Get<Entity, Window>> query,
    EventReader<CursorMove> cursor_read,
    EventWriter<CursorMove> cursor_event
);
void update_window_size(Query<Get<Window>> query);
void update_window_pos(Query<Get<Window>> query);
void close_window(
    Command command,
    EventReader<AnyWindowClose> any_close_event,
    Query<Get<Window>> query
);
void primary_window_close(
    Command command,
    Query<Get<Entity, Window>, With<PrimaryWindow>> query,
    EventWriter<AnyWindowClose> any_close_event
);
void window_close(
    Command command,
    Query<Get<Entity, Window>, Without<PrimaryWindow>> query,
    EventWriter<AnyWindowClose> any_close_event
);
void no_window_exists(
    Query<Get<Window>> query, EventWriter<NoWindowExists> no_window_event
);
void poll_events();
void scroll_events(
    EventReader<MouseScroll> scroll_read, EventWriter<MouseScroll> scroll_event
);
void exit_on_no_window(
    EventReader<NoWindowExists> no_window_event, EventWriter<AppExit> exit_event
);
}  // namespace systems
}  // namespace window
}  // namespace pixel_engine