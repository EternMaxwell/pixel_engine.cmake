﻿#include "pixel_engine/window/window.h"

#include "pixel_engine/window/systems.h"

void pixel_engine::window::systems::init_glfw() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
}

void pixel_engine::window::systems::create_window(
    Command command, Query<
                         Get<entt::entity, WindowHandle, const WindowSize, const WindowTitle, const WindowHints>,
                         With<>, Without<WindowCreated>>
                         query) {
    for (auto [id, window_handle, window_size, window_title, window_hints] : query.iter()) {
        spdlog::debug("Creating window");
        glfwDefaultWindowHints();

        for (auto& [hint, value] : window_hints.hints) {
            glfwWindowHint(hint, value);
        }

        window_handle.window_handle =
            glfwCreateWindow(window_size.width, window_size.height, window_title.title.c_str(), nullptr, nullptr);
        if (!window_handle.window_handle) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        command.entity(id).emplace(WindowCreated{});

        glfwShowWindow(window_handle.window_handle);
        glfwMakeContextCurrent(window_handle.window_handle);
        glfwSwapInterval(window_handle.vsync ? 1 : 0);
    }
}

void pixel_engine::window::systems::update_window_size(
    Query<Get<WindowHandle, WindowSize>, With<WindowCreated>, Without<>> query) {
    for (auto [window_handle, window_size] : query.iter()) {
        glfwGetWindowSize(window_handle.window_handle, &window_size.width, &window_size.height);
    }
}

void pixel_engine::window::systems::update_window_pos(
    Query<Get<WindowHandle, WindowPos>, With<WindowCreated>, Without<>> query) {
    for (auto [window_handle, window_pos] : query.iter()) {
        glfwGetWindowPos(window_handle.window_handle, &window_pos.x, &window_pos.y);
    }
}

void pixel_engine::window::systems::primary_window_close(
    Command command, Query<Get<entt::entity, WindowHandle>, With<WindowCreated, PrimaryWindow>, Without<>> query,
    EventWriter<AnyWindowClose> any_close_event) {
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
    Command command, Query<Get<entt::entity, WindowHandle>, With<WindowCreated>, Without<PrimaryWindow>> query,
    EventWriter<AnyWindowClose> any_close_event) {
    for (auto [entity, window_handle] : query.iter()) {
        if (glfwWindowShouldClose(window_handle.window_handle)) {
            glfwDestroyWindow(window_handle.window_handle);
            window_handle.window_handle = NULL;
            any_close_event.write(AnyWindowClose{});
            command.entity(entity).despawn();
        }
    }
}

void pixel_engine::window::systems::swap_buffers(Query<Get<const WindowHandle>, With<WindowCreated>, Without<>> query) {
    for (auto [window_handle] : query.iter()) {
        glfwSwapBuffers(window_handle.window_handle);
        glfwSwapInterval(window_handle.vsync ? 1 : 0);
    }
}

void pixel_engine::window::systems::no_window_exists(
    Query<Get<const WindowHandle>, With<>, Without<>> query, EventWriter<NoWindowExists> no_window_event) {
    for (auto [window_handle] : query.iter()) {
        if (window_handle.window_handle) return;
    }
    spdlog::info("No window exists");
    no_window_event.write(NoWindowExists{});
}

void pixel_engine::window::systems::poll_events() { glfwPollEvents(); }

void pixel_engine::window::systems::exit_on_no_window(
    EventReader<NoWindowExists> no_window_event, EventWriter<AppExit> exit_event) {
    for (auto& _ : no_window_event.read()) {
        glfwTerminate();
        exit_event.write(AppExit{});
        return;
    }
}
