#pragma once

namespace pixel_engine {
namespace input {
namespace systems {
using namespace components;
using namespace events;
using namespace prelude;
using namespace window::components;
using namespace window::events;

void create_input_for_window_start(
    Command command,
    Query<
        Get<Entity, const Window>,
        Without<ButtonInput<KeyCode>, ButtonInput<MouseButton>>> query
);
void create_input_for_window_update(
    Command command,
    Query<
        Get<Entity, const Window>,
        Without<ButtonInput<KeyCode>, ButtonInput<MouseButton>>> query

);
void update_input(
    Query<Get<Entity, ButtonInput<KeyCode>, ButtonInput<MouseButton>, const Window>>
        query,
    EventReader<events::KeyEvent> key_event_reader,
    EventWriter<events::KeyEvent> key_event_writer,
    EventReader<events::MouseButtonEvent> mouse_button_event_reader,
    EventWriter<events::MouseButtonEvent> mouse_button_event_writer
);
}  // namespace systems
}  // namespace input
}  // namespace pixel_engine