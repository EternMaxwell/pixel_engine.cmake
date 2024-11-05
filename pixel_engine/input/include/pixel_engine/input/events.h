#pragma once

#include <pixel_engine/app.h>
#include <pixel_engine/window.h>

#include "components.h"

namespace pixel_engine {
namespace input {
namespace events {
using namespace pixel_engine::prelude;
using MouseScroll = window::events::MouseScroll;
using CursorMove  = window::events::CursorMove;
enum ButtonState { Pressed, Released, Repeat };
struct KeyEvent {
    components::KeyCode key;
    ButtonState state;
    const char* key_name;
    Entity window;
};
struct MouseButtonEvent {
    components::MouseButton button;
    ButtonState state;
    Entity window;
};
}  // namespace events
}  // namespace input
}  // namespace pixel_engine