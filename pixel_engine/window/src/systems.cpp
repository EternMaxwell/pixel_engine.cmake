#include "pixel_engine/window/resources.h"
#include "pixel_engine/window/systems.h"
#include "systems.h"

using namespace pixel_engine::window::systems;
using namespace pixel_engine::window;
using namespace pixel_engine::prelude;

void systems::init_glfw() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
}

void systems::insert_window_map(Command command) {
    command.insert_resource(resources::WindowMap{});
}

void systems::insert_primary_window(
    Command command, Resource<window::WindowPlugin> window_plugin
) {
    command.spawn(window_plugin->primary_desc());
}

void systems::create_window_start(
    Command command,
    Query<Get<Entity, const WindowDescription>, Without<Window>> desc_query
) {
    for (auto [entity, desc] : desc_query.iter()) {
        spdlog::debug("create window {} at startup.", desc.title);
        auto window = components::create_window(desc);
        command.entity(entity).erase<WindowDescription>();
        if (window.has_value()) {
            command.entity(entity).insert(window.value());
        } else {
            spdlog::error(
                "Failed to create window {} with size {}x{}", desc.title,
                desc.width, desc.height
            );
        }
    }
}

void systems::create_window_update(
    Command command,
    Query<Get<Entity, const WindowDescription>, Without<Window>> desc_query
) {
    for (auto [entity, desc] : desc_query.iter()) {
        spdlog::debug("create window {} at update.", desc.title);
        auto window = components::create_window(desc);
        command.entity(entity).erase<WindowDescription>();
        if (window.has_value()) {
            command.entity(entity).insert(window.value());
        } else {
            spdlog::error(
                "Failed to create window {} with size {}x{}", desc.title,
                desc.width, desc.height
            );
        }
    }
}

void systems::update_window_size(Query<Get<Window>> query) {
    for (auto [window] : query.iter()) {
        update_size(window);
    }
}

void systems::update_window_pos(Query<Get<Window>> query) {
    for (auto [window] : query.iter()) {
        update_pos(window);
    }
}

void systems::primary_window_close(
    Command command,
    Query<Get<Entity, Window>, With<PrimaryWindow>> query,
    EventWriter<AnyWindowClose> any_close_event
) {
    for (auto [entity, window] : query.iter()) {
        if (window.should_close()) {
            window.destroy();
            any_close_event.write(AnyWindowClose{entity});
        }
    }
}

void systems::window_close(
    Command command,
    Query<Get<Entity, Window>, Without<PrimaryWindow>> query,
    EventWriter<AnyWindowClose> any_close_event
) {
    for (auto [entity, window] : query.iter()) {
        if (window.should_close()) {
            window.destroy();
            any_close_event.write(AnyWindowClose{entity});
        }
    }
}

void systems::no_window_exists(
    Query<Get<Window>> query, EventWriter<NoWindowExists> no_window_event
) {
    for (auto [window] : query.iter()) {
        if (window.get_handle()) return;
    }
    spdlog::info("No window exists.");
    no_window_event.write(NoWindowExists{});
}

void systems::poll_events() { glfwPollEvents(); }

void systems::exit_on_no_window(
    EventReader<NoWindowExists> no_window_event, EventWriter<AppExit> exit_event
) {
    for (auto _ : no_window_event.iter()) {
        exit_event.write(AppExit{});
    }
}
