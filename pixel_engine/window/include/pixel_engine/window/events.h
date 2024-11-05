﻿#pragma once

#include <pixel_engine/app.h>

namespace pixel_engine {
namespace window {
namespace components {
struct Window;
}
namespace events {
using namespace prelude;
using namespace components;
struct AnyWindowClose {
    Entity window;
};
struct NoWindowExists {};
struct PrimaryWindowClose {};

struct MouseScroll {
    double xoffset;
    double yoffset;
    Entity window;
};
struct CursorMove {
    double x;
    double y;
    Entity window;
};
}  // namespace events
}  // namespace window
}  // namespace pixel_engine