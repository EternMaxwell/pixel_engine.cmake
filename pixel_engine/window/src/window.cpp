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
    app.configure_sets(
           WindowStartUpSets::glfw_initialization,
           WindowStartUpSets::window_creation,
           WindowStartUpSets::after_window_creation
    )
        .add_system(insert_primary_window)
        .in_stage(app::PreStartup)
        .use_worker("single")
        ->add_system(init_glfw)
        .in_stage(app::PreStartup)
        .in_set(WindowStartUpSets::glfw_initialization)
        .use_worker("single")
        ->add_system(create_window_startup)
        .in_stage(app::PreStartup)
        .after(insert_primary_window)
        .in_set(WindowStartUpSets::window_creation)
        .use_worker("single")
        ->configure_sets(
            WindowPreUpdateSets::poll_events,
            WindowPreUpdateSets::update_window_data
        )
        ->add_system(poll_events)
        .in_stage(app::First)
        .in_set(WindowPreUpdateSets::poll_events)
        .use_worker("single")
        ->add_system(update_window_size)
        .in_stage(app::First)
        .in_set(WindowPreUpdateSets::update_window_data)
        .use_worker("single")
        ->add_system(update_window_pos)
        .in_stage(app::First)
        .in_set(WindowPreUpdateSets::update_window_data)
        .use_worker("single")
        ->configure_sets(
            WindowPreRenderSets::before_create,
            WindowPreRenderSets::window_creation,
            WindowPreRenderSets::after_create
        )
        ->add_system(create_window_prerender)
        .in_stage(app::Prepare)
        .in_set(WindowPreRenderSets::window_creation)
        .use_worker("single")
        ->configure_sets(
            WindowPostRenderSets::before_swap_buffers,
            WindowPostRenderSets::swap_buffers,
            WindowPostRenderSets::window_close,
            WindowPostRenderSets::after_close_window
        )
        ->add_system(swap_buffers)
        .in_stage(app::PostRender)
        .in_set(WindowPostRenderSets::swap_buffers)
        .use_worker("single")
        ->add_system(primary_window_close)
        .in_stage(app::PostRender)
        .in_set(WindowPostRenderSets::window_close)
        .use_worker("single")
        ->add_system(window_close)
        .in_stage(app::PostRender)
        .in_set(WindowPostRenderSets::window_close)
        .use_worker("single")
        ->add_system(no_window_exists)
        .in_stage(app::PostRender)
        .in_set(WindowPostRenderSets::after_close_window)
        ->add_system(exit_on_no_window)
        .in_stage(app::PostRender)
        .after(no_window_exists)
        .in_set(WindowPostRenderSets::after_close_window);
}
