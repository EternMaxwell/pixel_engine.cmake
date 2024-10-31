#include <spdlog/spdlog.h>

#include "pixel_engine/input.h"

using namespace pixel_engine;
using namespace pixel_engine::input;
using namespace pixel_engine::prelude;
using namespace pixel_engine::input::systems;

void systems::create_input_for_window_start(
    Command command,
    Query<
        Get<Entity, const window::components::Window>,
        Without<ButtonInput<KeyCode>, ButtonInput<MouseButton>>> query
) {
    for (auto [entity, window] : query.iter()) {
        spdlog::debug("create input for window {} at startup.", window.m_title);
        command.entity(entity).emplace(
            ButtonInput<KeyCode>(Handle<Window>{entity})
        );
        command.entity(entity).emplace(
            ButtonInput<MouseButton>(Handle<Window>{entity})
        );
    }
}

void systems::create_input_for_window_update(
    Command command,
    Query<
        Get<Entity, const window::components::Window>,
        Without<ButtonInput<KeyCode>, ButtonInput<MouseButton>>> query
) {
    for (auto [entity, window] : query.iter()) {
        spdlog::debug("create input for window {} at update.", window.m_title);
        command.entity(entity).emplace(
            ButtonInput<KeyCode>(Handle<Window>{entity})
        );
        command.entity(entity).emplace(
            ButtonInput<MouseButton>(Handle<Window>{entity})
        );
    }
}

void systems::update_input(Query<
                           Get<ButtonInput<KeyCode>,
                               ButtonInput<MouseButton>,
                               const window::components::Window>> query) {
    for (auto [key_input, mouse_input, window] : query.iter()) {
        if (window.get_handle() == nullptr) {
            continue;
        }
        update_key_button_input(key_input, window.get_handle());
        update_mouse_button_input(mouse_input, window.get_handle());
    }
}
