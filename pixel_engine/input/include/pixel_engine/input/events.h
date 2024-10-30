#pragma once

#include <pixel_engine/app.h>
#include <pixel_engine/window.h>

namespace pixel_engine {
namespace input {
namespace events {
using namespace prelude;
struct MouseMotion {
    struct ivec2 {
        int x;
        int y;
    };

    ivec2 position;
    ivec2 delta;
};

struct MouseScroll {
    struct ivec2 {
        int x;
        int y;
    };

    ivec2 offset;
};

struct CursorMoved {
    struct dvec2 {
        double x;
        double y;
    };

    dvec2 position;
    const GLFWwindow* window;
};
}  // namespace events
}  // namespace input
}  // namespace pixel_engine