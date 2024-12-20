#pragma once

#include <pixel_engine/app.h>
#include <pixel_engine/window.h>

#include "input/components.h"
#include "input/events.h"
#include "input/systems.h"

namespace pixel_engine {
namespace input {
using namespace prelude;
using namespace systems;
using namespace events;

struct InputPlugin : Plugin {
    bool enable_output_event = false;
    InputPlugin& enable_output();
    InputPlugin& disable_output();
    void build(App& app) override;
};
}  // namespace input
}  // namespace pixel_engine
