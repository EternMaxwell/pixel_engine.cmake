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
    Handle<window::components::Window> window;
};
struct MouseButtonEvent {
    components::MouseButton button;
    ButtonState state;
    Handle<window::components::Window> window;
};
}  // namespace events
}  // namespace input
}  // namespace pixel_engine