#pragma once

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
    Handle<Window> handle;
};
struct NoWindowExists {};
struct PrimaryWindowClose {};
}  // namespace events
}  // namespace window
}  // namespace pixel_engine