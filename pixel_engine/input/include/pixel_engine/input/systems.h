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
    Query<Get<ButtonInput<KeyCode>, ButtonInput<MouseButton>, const Window>>
        query
);
}  // namespace systems
}  // namespace input
}  // namespace pixel_engine