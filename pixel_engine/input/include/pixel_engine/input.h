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
    void build(App& app) override {
        app.add_event<events::KeyEvent>();
        app.add_event<events::MouseButtonEvent>();
        app.add_system(app::Startup, create_input_for_window_start)
            ->add_system(app::First, create_input_for_window_update)
            .after(window::systems::poll_events)
            .use_worker("single")
            ->add_system(app::First, update_input)
            .after(window::systems::poll_events)
            .use_worker("single");
    }
};
}  // namespace input
}  // namespace pixel_engine
