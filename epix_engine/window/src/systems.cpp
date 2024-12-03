#include "epix/window.h"
#include "epix/window/resources.h"
#include "epix/window/systems.h"
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

using namespace epix::window::systems;
using namespace epix::window;
using namespace epix::prelude;

EPIX_API void systems::init_glfw() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
}

EPIX_API void systems::create_window_thread_pool(Command command) {
    command.emplace_resource<resources::WindowThreadPool>();
}

EPIX_API void systems::insert_primary_window(
    Command command, ResMut<window::WindowPlugin> window_plugin
) {
    command.spawn(window_plugin->primary_desc(), PrimaryWindow{});
}

static std::mutex scroll_mutex;
static std::vector<events::MouseScroll> scroll_cache;

EPIX_API void scroll_callback(
    GLFWwindow* window, double xoffset, double yoffset
) {
    std::lock_guard<std::mutex> lock(scroll_mutex);
    std::pair<Entity, resources::WindowThreadPool*>* ptr =
        static_cast<std::pair<Entity, resources::WindowThreadPool*>*>(
            glfwGetWindowUserPointer(window)
        );
    scroll_cache.emplace_back(xoffset, yoffset, ptr->first);
}

EPIX_API void systems::create_window(
    Command command,
    Query<Get<Entity, const WindowDescription>, Without<Window>> desc_query,
    ResMut<resources::WindowThreadPool> pool
) {
    for (auto [entity, desc] : desc_query.iter()) {
        spdlog::debug("create window {}.", desc.title);
        auto window = pool->submit_task([desc]() {
                              return components::create_window(desc);
                          }
        ).get();
        command.entity(entity).erase<WindowDescription>();
        if (window.has_value()) {
            command.entity(entity).emplace(window.value());
            auto* ptr = new std::pair<Entity, resources::WindowThreadPool*>{
                entity, &(*pool)
            };
            glfwSetWindowUserPointer(
                window.value().get_handle(), static_cast<void*>(ptr)
            );
            glfwSetScrollCallback(window.value().get_handle(), scroll_callback);
        } else {
            spdlog::error(
                "Failed to create window {} with size {}x{}", desc.title,
                desc.width, desc.height
            );
        }
    }
}

EPIX_API void systems::update_window_cursor_pos(
    Query<Get<Entity, Window>> query,
    EventReader<events::CursorMove> cursor_read,
    EventWriter<events::CursorMove> cursor_event
) {
    cursor_read.clear();
    for (auto [entity, window] : query.iter()) {
        update_cursor(window);
        if (window.get_cursor_move().has_value()) {
            auto [x, y] = window.get_cursor_move().value();
            cursor_event.write(events::CursorMove{x, y, entity});
        }
    }
}

EPIX_API void systems::update_window_size(Query<Get<Window>> query) {
    for (auto [window] : query.iter()) {
        update_size(window);
    }
}

EPIX_API void systems::update_window_pos(Query<Get<Window>> query) {
    for (auto [window] : query.iter()) {
        update_pos(window);
    }
}

EPIX_API void systems::close_window(
    Command command,
    EventReader<AnyWindowClose> any_close_event,
    Query<Get<Window>> query
) {
    for (auto [entity] : any_close_event.read()) {
        auto [window] = query.get(entity);
        window.destroy();
        command.entity(entity).despawn();
    }
}

EPIX_API void systems::primary_window_close(
    Command command,
    Query<Get<Entity, Window>, With<PrimaryWindow>> query,
    EventWriter<AnyWindowClose> any_close_event
) {
    for (auto [entity, window] : query.iter()) {
        if (window.should_close()) {
            any_close_event.write(AnyWindowClose{entity});
        }
    }
}

EPIX_API void systems::window_close(
    Command command,
    Query<Get<Entity, Window>, Without<PrimaryWindow>> query,
    EventWriter<AnyWindowClose> any_close_event
) {
    for (auto [entity, window] : query.iter()) {
        if (window.should_close()) {
            any_close_event.write(AnyWindowClose{entity});
        }
    }
}

EPIX_API void systems::no_window_exists(
    Query<Get<Window>> query, EventWriter<NoWindowExists> no_window_event
) {
    for (auto [window] : query.iter()) {
        if (!window.should_close()) return;
    }
    spdlog::info("No window exists.");
    no_window_event.write(NoWindowExists{});
}

EPIX_API void systems::poll_events(
    ResMut<resources::WindowThreadPool> pool,
    Local<std::future<void>> future,
    Query<Get<Window>, With<PrimaryWindow>> query
) {
    if (!query.single().has_value()) return;
    auto [window] = query.single().value();
    if (!future->valid() || future->wait_for(std::chrono::milliseconds(0)) ==
                                std::future_status::ready) {
        (*future) = pool->submit_task([]() { glfwPollEvents(); });
    }
    if (future->wait_for(std::chrono::nanoseconds(0)) ==
        std::future_status::ready) {
        future->get();
    }
}

EPIX_API void systems::scroll_events(
    EventReader<MouseScroll> scroll_read, EventWriter<MouseScroll> scroll_event
) {
    scroll_read.clear();
    std::lock_guard<std::mutex> lock(scroll_mutex);
    for (auto& scroll : scroll_cache) {
        scroll_event.write(scroll);
    }
    scroll_cache.clear();
}

EPIX_API void systems::exit_on_no_window(
    EventReader<NoWindowExists> no_window_event, EventWriter<AppExit> exit_event
) {
    for (auto _ : no_window_event.read()) {
        exit_event.write(AppExit{});
    }
}