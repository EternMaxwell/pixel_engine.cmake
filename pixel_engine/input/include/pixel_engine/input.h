#pragma once

#include <pixel_engine/app.h>

#include "input/components.h"
#include "input/events.h"
#include "input/resources.h"
#include "input/systems.h"

namespace pixel_engine {
namespace input {
using namespace prelude;

struct InputPlugin : Plugin {
    void build(App& app) override {}
};
}  // namespace input
}  // namespace pixel_engine
