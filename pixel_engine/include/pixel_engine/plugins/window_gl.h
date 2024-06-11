#pragma once

#include <GLFW/glfw3.h>

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace plugins {
        using namespace entity;

        namespace window_gl {

            struct AnyWindowClose {};
            struct NoWindowExists {};
            struct PrimaryWindowClose {};

            struct WindowHandle {
                GLFWwindow* window_handle = NULL;
                bool vsync = true;
            };

            struct WindowCreated {};

            struct PrimaryWindow {};

            struct WindowSize {
                int width, height;
            };

            struct WindowPos {
                int x, y;
            };

            struct WindowTitle {
                std::string title;
            };

            struct WindowHints {
                std::vector<std::pair<int, int>> hints;
            };

            struct WindowBundle {
                Bundle bundle;
                WindowHandle window_handle;
                WindowSize window_size = {480 * 3, 270 * 3};
                WindowPos window_pos = {0, 0};
                WindowTitle window_title = {"Pixel Engine"};
                WindowHints window_hints = {.hints = {{GLFW_CONTEXT_VERSION_MAJOR, 4},
                                                      {GLFW_CONTEXT_VERSION_MINOR, 5},
                                                      {GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE},
                                                      {GLFW_VISIBLE, GLFW_FALSE}}};
            };

            void init_glfw() {
                if (!glfwInit()) {
                    throw std::runtime_error("Failed to initialize GLFW");
                }
            }

            void create_window(
                Command command,
                Query<std::tuple<entt::entity, WindowHandle, const WindowSize, const WindowTitle, const WindowHints>,
                      std::tuple<WindowCreated>>
                    query) {
                for (auto [id, window_handle, window_size, window_title, window_hints] : query.iter()) {
                    glfwDefaultWindowHints();

                    for (auto& [hint, value] : window_hints.hints) {
                        glfwWindowHint(hint, value);
                    }

                    window_handle.window_handle = glfwCreateWindow(window_size.width, window_size.height,
                                                                   window_title.title.c_str(), nullptr, nullptr);
                    if (!window_handle.window_handle) {
                        glfwTerminate();
                        throw std::runtime_error("Failed to create GLFW window");
                    }

                    command.entity(id).emplace(WindowCreated{});

                    glfwShowWindow(window_handle.window_handle);
                }
            }

            void update_window_size(
                Query<std::tuple<WindowHandle, WindowSize, const WindowCreated>, std::tuple<>> query) {
                for (auto [window_handle, window_size] : query.iter()) {
                    glfwGetWindowSize(window_handle.window_handle, &window_size.width, &window_size.height);
                }
            }

            void update_window_pos(
                Query<std::tuple<WindowHandle, WindowPos, const WindowCreated>, std::tuple<>> query) {
                for (auto [window_handle, window_pos] : query.iter()) {
                    glfwGetWindowPos(window_handle.window_handle, &window_pos.x, &window_pos.y);
                }
            }

            void primary_window_close(
                Command command,
                Query<std::tuple<entt::entity, WindowHandle, PrimaryWindow, const WindowCreated>, std::tuple<>> query) {
                for (auto [entity, window_handle] : query.iter()) {
                    if (glfwWindowShouldClose(window_handle.window_handle)) {
                        glfwDestroyWindow(window_handle.window_handle);
                        command.entity(entity).despawn();
                    }
                }
            }

            void window_close(
                Command command,
                Query<std::tuple<entt::entity, const WindowHandle, const WindowCreated>, std::tuple<PrimaryWindow>>
                    query,
                EventWriter<AnyWindowClose> any_close_event) {
                for (auto [entity, window_handle] : query.iter()) {
                    if (glfwWindowShouldClose(window_handle.window_handle)) {
                        glfwDestroyWindow(window_handle.window_handle);
                        any_close_event.write(AnyWindowClose{});
                        command.entity(entity).despawn();
                    }
                }
            }

            void swap_buffers(Query<std::tuple<const WindowHandle, const WindowCreated>, std::tuple<>> query) {
                for (auto [window_handle] : query.iter()) {
                    glfwSwapBuffers(window_handle.window_handle);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                }
            }

            void no_window_exists(Query<std::tuple<const WindowHandle>, std::tuple<>> query,
                                  EventWriter<NoWindowExists> no_window_event) {
                for (auto [window_handle] : query.iter()) {
                    return;
                }
                spdlog::info("No window exists");
                no_window_event.write(NoWindowExists{});
            }

            void make_context_primary(
                Query<std::tuple<const WindowHandle, const PrimaryWindow, const WindowCreated>, std::tuple<>> query) {
                for (auto [window_handle] : query.iter()) {
                    if (glfwGetCurrentContext() != window_handle.window_handle) {
                        glfwMakeContextCurrent(window_handle.window_handle);
                    }
                    glfwSwapInterval(window_handle.vsync ? 1 : 0);
                }
            }

            void poll_events() { glfwPollEvents(); }

            void exit_on_no_window(EventReader<NoWindowExists> no_window_event, EventWriter<AppExit> exit_event) {
                for (auto& _ : no_window_event.read()) {
                    glfwTerminate();
                    exit_event.write(AppExit{});
                    return;
                }
            }

        }  // namespace window_gl

        class WindowGLPlugin : public Plugin {
           private:
            int primary_window_width = 480 * 3;
            int primary_window_height = 270 * 3;
            const char* primary_window_title = "Pixel Engine";

            std::function<void(Command)> insert_primary_window = [&](Command command) {
                command.spawn(window_gl::WindowBundle{.window_size = {.width = primary_window_width,
                                                                      .height = primary_window_height},
                                                      .window_title = {.title = primary_window_title}},
                              window_gl::PrimaryWindow{});
            };

           public:
            Node start_up_window_create_node;
            Node swap_buffer_node;
            void set_primary_window_size(int width, int height) {
                primary_window_width = width;
                primary_window_height = height;
            }
            void set_primary_window_title(const std::string& title) { primary_window_title = title.c_str(); }
            void build(App& app) override {
                Node init_glfw_node, create_window_node, window_close_node, no_window_exists_node, insert_primary_node,
                    make_context_primary_node, primary_window_close_node, poll_events_node;
                app.add_system_main(Startup{}, insert_primary_window, &insert_primary_node)
                    .add_system_main(Startup{}, window_gl::init_glfw, &init_glfw_node)
                    .add_system_main(Startup{}, window_gl::create_window, &start_up_window_create_node,
                                     before(init_glfw_node, insert_primary_node))
                    .add_system_main(PreUpdate{}, window_gl::poll_events, &poll_events_node)
                    .add_system_main(PreUpdate{}, window_gl::update_window_size, before(poll_events_node))
                    .add_system_main(PreUpdate{}, window_gl::update_window_pos, before(poll_events_node))
                    .add_system_main(PreRender{}, window_gl::create_window, &create_window_node)
                    .add_system_main(PostRender{}, window_gl::make_context_primary, &make_context_primary_node)
                    .add_system_main(PostRender{}, window_gl::swap_buffers, &swap_buffer_node,
                                     before(make_context_primary_node))
                    .add_system_main(PostRender{}, window_gl::primary_window_close, &primary_window_close_node,
                                     before(make_context_primary_node, swap_buffer_node))
                    .add_system_main(PostRender{}, window_gl::window_close, &window_close_node,
                                     before(create_window_node, swap_buffer_node))
                    .add_system(PostRender{}, window_gl::no_window_exists, &no_window_exists_node,
                                before{window_close_node, primary_window_close_node})
                    .add_system(PostRender{}, window_gl::exit_on_no_window, before{no_window_exists_node});
            }
        };
    }  // namespace plugins
}  // namespace pixel_engine