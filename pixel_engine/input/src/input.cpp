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
            ButtonInput<KeyCode>(entity)
        );
        command.entity(entity).emplace(
            ButtonInput<MouseButton>(entity)
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
            ButtonInput<KeyCode>(entity)
        );
        command.entity(entity).emplace(
            ButtonInput<MouseButton>(entity)
        );
    }
}

void systems::update_input(
    Query<
        Get<Entity,
            ButtonInput<KeyCode>,
            ButtonInput<MouseButton>,
            const window::components::Window>> query,
    EventReader<events::KeyEvent> key_event_reader,
    EventWriter<events::KeyEvent> key_event_writer,
    EventReader<events::MouseButtonEvent> mouse_button_event_reader,
    EventWriter<events::MouseButtonEvent> mouse_button_event_writer
) {
    for (auto [entity, key_input, mouse_input, window] : query.iter()) {
        if (window.get_handle() == nullptr) {
            continue;
        }
        update_key_button_input(key_input, window.get_handle());
        update_mouse_button_input(mouse_input, window.get_handle());
    }
    key_event_reader.clear();
    mouse_button_event_reader.clear();
    for (auto [entity, key_input, mouse_input, window] : query.iter()) {
        for (auto key : key_input.just_pressed_keys()) {
            key_event_writer.write(events::KeyEvent{
                key, ButtonState::Pressed, key_input.key_name(key), entity
            });
        }
        for (auto key : key_input.pressed_keys()) {
            if (key_input.just_pressed_keys().find(key) ==
                key_input.just_pressed_keys().end()) {
                key_event_writer.write(events::KeyEvent{
                    key, ButtonState::Repeat, key_input.key_name(key), entity
                });
            }
        }
        for (auto key : key_input.just_released_keys()) {
            key_event_writer.write(events::KeyEvent{
                key, ButtonState::Released, key_input.key_name(key), entity
            });
        }
        for (auto button : mouse_input.just_pressed_buttons()) {
            mouse_button_event_writer.write(
                events::MouseButtonEvent{button, ButtonState::Pressed, entity}
            );
        }
        for (auto button : mouse_input.pressed_buttons()) {
            if (mouse_input.just_pressed_buttons().find(button) ==
                mouse_input.just_pressed_buttons().end()) {
                mouse_button_event_writer.write(events::MouseButtonEvent{
                    button, ButtonState::Repeat, entity
                });
            }
        }
        for (auto button : mouse_input.just_released_buttons()) {
            mouse_button_event_writer.write(
                events::MouseButtonEvent{button, ButtonState::Released, entity}
            );
        }
    }
}