#include "pixel_engine/window.h"
#include "pixel_engine/window/systems.h"

using namespace pixel_engine::window::systems;
using namespace pixel_engine::window::events;

using namespace pixel_engine::window;
using namespace pixel_engine::prelude;

WindowDescription& WindowPlugin::primary_desc() {
    return primary_window_description;
}

void pixel_engine::window::WindowPlugin::build(App& app) {
    app->configure_sets(
           WindowStartUpSets::glfw_initialization,
           WindowStartUpSets::window_creation,
           WindowStartUpSets::after_window_creation
    )
        ->configure_sets(
            WindowPreRenderSets::before_create,
            WindowPreRenderSets::window_creation,
            WindowPreRenderSets::after_create
        )
        ->add_system(app::PreStartup, init_glfw)
        .in_set(WindowStartUpSets::glfw_initialization)
        .use_worker("single")
        ->add_system(app::PreStartup, insert_primary_window)
        .before(create_window_start)
        ->add_system(app::PreStartup, create_window_start)
        .in_set(WindowStartUpSets::window_creation)
        .use_worker("single")
        ->add_system(app::First, poll_events)
        .use_worker("single")
        ->add_system(app::First, scroll_events)
        .after(poll_events)
        ->add_system(app::First, update_window_cursor_pos)
        .after(poll_events)
        .before(close_window)
        .use_worker("single")
        ->add_system(app::First, update_window_size)
        .after(poll_events)
        .before(close_window)
        .use_worker("single")
        ->add_system(app::First, update_window_pos)
        .after(poll_events)
        .before(close_window)
        .use_worker("single")
        ->add_system(app::First, close_window)
        ->add_system(app::Last, primary_window_close)
        .before(no_window_exists)
        .use_worker("single")
        ->add_system(app::Last, window_close)
        .before(no_window_exists)
        .use_worker("single")
        ->add_system(app::Last, no_window_exists)
        .before(exit_on_no_window)
        .use_worker("single")
        ->add_system(app::Last, exit_on_no_window);
}
