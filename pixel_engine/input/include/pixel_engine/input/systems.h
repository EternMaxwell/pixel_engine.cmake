#pragma once

namespace pixel_engine {
namespace input {
namespace systems {
using namespace components;
using namespace events;
using namespace prelude;
using namespace window::components;
using namespace window::events;

void create_input_for_window(
    Command command,
    Query<
        Get<Entity, const Window>,
        Without<ButtonInput<KeyCode>, ButtonInput<MouseButton>>> query
);
void update_input(
    Query<
        Get<Entity,
            ButtonInput<KeyCode>,
            ButtonInput<MouseButton>,
            const Window>> query,
    EventReader<events::KeyEvent> key_event_reader,
    EventWriter<events::KeyEvent> key_event_writer,
    EventReader<events::MouseButtonEvent> mouse_button_event_reader,
    EventWriter<events::MouseButtonEvent> mouse_button_event_writer
);

void output_event(
    EventReader<pixel_engine::input::events::MouseScroll> scroll_events,
    Query<Get<ButtonInput<KeyCode>, ButtonInput<MouseButton>, const Window>>
        query,
    EventReader<pixel_engine::input::events::CursorMove> cursor_move_events
);
}  // namespace systems
}  // namespace input
}  // namespace pixel_engine