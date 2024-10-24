#include "pixel_engine/window.h"
#include "pixel_engine/window/systems.h"

using namespace pixel_engine::window::systems;
using namespace pixel_engine::window::events;

using namespace pixel_engine::window;
using namespace pixel_engine::prelude;

void pixel_engine::window::systems::init_glfw() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
}

void pixel_engine::window::systems::insert_primary_window(
    Command command, Resource<WindowPlugin> window_plugin
) {
    using namespace pixel_engine::window::components;
    command.spawn(
        WindowBundle{
            .window_handle{
                .window_handle = nullptr,
                .vsync = window_plugin->primary_window_vsync
            },
            .window_size{
                window_plugin->primary_window_width,
                window_plugin->primary_window_height
            },
            .window_title{window_plugin->primary_window_title},
            .window_hints{window_plugin->window_hints},
        },
        PrimaryWindow{}
    );
}

void pixel_engine::window::systems::create_window_startup(
    Command command,
    Query<
        Get<entt::entity,
            WindowHandle,
            const WindowSize,
            const WindowTitle,
            const WindowHints>,
        With<>,
        Without<WindowCreated>> query
) {
    for (auto [id, window_handle, window_size, window_title, window_hints] :
         query.iter()) {
        spdlog::debug("Creating window startup");
        glfwDefaultWindowHints();

        for (auto& [hint, value] : window_hints.hints) {
            glfwWindowHint(hint, value);
        }

        window_handle.window_handle = glfwCreateWindow(
            window_size.width, window_size.height, window_title.title.c_str(),
            nullptr, nullptr
        );
        if (!window_handle.window_handle) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        command.entity(id).emplace(WindowCreated{});

        glfwShowWindow(window_handle.window_handle);
        if (glfwGetWindowAttrib(window_handle.window_handle, GLFW_CLIENT_API) ==
            GLFW_NO_API)
            continue;
        glfwMakeContextCurrent(window_handle.window_handle);
        glfwSwapInterval(window_handle.vsync ? 1 : 0);
    }
}

void pixel_engine::window::systems::create_window_prerender(
    Command command,
    Query<
        Get<entt::entity,
            WindowHandle,
            const WindowSize,
            const WindowTitle,
            const WindowHints>,
        With<>,
        Without<WindowCreated>> query
) {
    for (auto [id, window_handle, window_size, window_title, window_hints] :
         query.iter()) {
        spdlog::debug("Creating window prerender");
        glfwDefaultWindowHints();

        for (auto& [hint, value] : window_hints.hints) {
            glfwWindowHint(hint, value);
        }

        window_handle.window_handle = glfwCreateWindow(
            window_size.width, window_size.height, window_title.title.c_str(),
            nullptr, nullptr
        );
        if (!window_handle.window_handle) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        command.entity(id).emplace(WindowCreated{});

        glfwShowWindow(window_handle.window_handle);
        if (glfwGetWindowAttrib(window_handle.window_handle, GLFW_CLIENT_API) ==
            GLFW_NO_API)
            continue;
        glfwMakeContextCurrent(window_handle.window_handle);
        glfwSwapInterval(window_handle.vsync ? 1 : 0);
    }
}

void pixel_engine::window::systems::update_window_size(
    Query<Get<WindowHandle, WindowSize>, With<WindowCreated>, Without<>> query
) {
    for (auto [window_handle, window_size] : query.iter()) {
        glfwGetWindowSize(
            window_handle.window_handle, &window_size.width, &window_size.height
        );
    }
}

void pixel_engine::window::systems::update_window_pos(
    Query<Get<WindowHandle, WindowPos>, With<WindowCreated>, Without<>> query
) {
    for (auto [window_handle, window_pos] : query.iter()) {
        glfwGetWindowPos(
            window_handle.window_handle, &window_pos.x, &window_pos.y
        );
    }
}

void pixel_engine::window::systems::primary_window_close(
    Command command,
    Query<
        Get<entt::entity, WindowHandle>,
        With<WindowCreated, PrimaryWindow>,
        Without<>> query,
    EventWriter<AnyWindowClose> any_close_event
) {
    for (auto [entity, window_handle] : query.iter()) {
        if (glfwWindowShouldClose(window_handle.window_handle)) {
            glfwDestroyWindow(window_handle.window_handle);
            window_handle.window_handle = NULL;
            any_close_event.write(AnyWindowClose{});
            command.entity(entity).despawn();
        }
    }
}

void pixel_engine::window::systems::window_close(
    Command command,
    Query<
        Get<entt::entity, WindowHandle>,
        With<WindowCreated>,
        Without<PrimaryWindow>> query,
    EventWriter<AnyWindowClose> any_close_event
) {
    for (auto [entity, window_handle] : query.iter()) {
        if (glfwWindowShouldClose(window_handle.window_handle)) {
            glfwDestroyWindow(window_handle.window_handle);
            window_handle.window_handle = NULL;
            any_close_event.write(AnyWindowClose{});
            command.entity(entity).despawn();
        }
    }
}

void pixel_engine::window::systems::swap_buffers(
    Query<Get<const WindowHandle>, With<WindowCreated>, Without<>> query
) {
    for (auto [window_handle] : query.iter()) {
        auto api =
            glfwGetWindowAttrib(window_handle.window_handle, GLFW_CLIENT_API);
        if (api == GLFW_NO_API) continue;
        glfwSwapBuffers(window_handle.window_handle);
        glfwSwapInterval(window_handle.vsync ? 1 : 0);
    }
}

void pixel_engine::window::systems::no_window_exists(
    Query<Get<const WindowHandle>, With<>, Without<>> query,
    EventWriter<NoWindowExists> no_window_event
) {
    for (auto [window_handle] : query.iter()) {
        if (window_handle.window_handle) return;
    }
    spdlog::info("No window exists");
    no_window_event.write(NoWindowExists{});
}

void pixel_engine::window::systems::poll_events() { glfwPollEvents(); }

void pixel_engine::window::systems::exit_on_no_window(
    EventReader<NoWindowExists> no_window_event, EventWriter<AppExit> exit_event
) {
    for (auto& _ : no_window_event.read()) {
        glfwTerminate();
        exit_event.write(AppExit{});
        return;
    }
}

void pixel_engine::window::WindowPlugin::set_primary_window_size(
    int width, int height
) {
    primary_window_width = width;
    primary_window_height = height;
}

void pixel_engine::window::WindowPlugin::set_primary_window_title(
    const std::string& title
) {
    primary_window_title = title.c_str();
}

void pixel_engine::window::WindowPlugin::set_primary_window_hints(
    const std::vector<std::pair<int, int>>& hints
) {
    window_hints.hints = hints;
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
