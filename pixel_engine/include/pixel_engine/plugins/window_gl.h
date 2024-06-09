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
                bool created = false;
            };

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
                Query<std::tuple<WindowHandle, WindowSize, WindowPos, WindowTitle, WindowHints>, std::tuple<>> query) {
                for (auto [window_handle, window_size, window_pos, window_title, window_hints] : query.iter()) {
                    if (!window_handle.created) {
                        glfwDefaultWindowHints();

                        for (auto [hint, value] : window_hints.hints) {
                            glfwWindowHint(hint, value);
                        }

                        window_handle.window_handle = glfwCreateWindow(window_size.width, window_size.height,
                                                                       window_title.title.c_str(), nullptr, nullptr);
                        if (!window_handle.window_handle) {
                            glfwTerminate();
                            throw std::runtime_error("Failed to create GLFW window");
                        }

                        window_handle.created = true;

                        glfwShowWindow(window_handle.window_handle);
                    }
                }
            }

            void primary_window_close(
                Command command, Query<std::tuple<entt::entity, WindowHandle, PrimaryWindow>, std::tuple<>> query) {
                for (auto [entity, window_handle] : query.iter()) {
                    if (window_handle.created) {
                        if (glfwWindowShouldClose(window_handle.window_handle)) {
                            glfwDestroyWindow(window_handle.window_handle);
                            command.entity(entity).despawn();
                        }
                    }
                }
            }

            void window_close(Command command,
                              Query<std::tuple<entt::entity, WindowHandle>, std::tuple<PrimaryWindow>> query,
                              EventWriter<AnyWindowClose> any_close_event) {
                for (auto [entity, window_handle] : query.iter()) {
                    if (window_handle.created) {
                        if (glfwWindowShouldClose(window_handle.window_handle)) {
                            glfwDestroyWindow(window_handle.window_handle);
                            any_close_event.write(AnyWindowClose{});
                            command.entity(entity).despawn();
                        }
                    }
                }
            }

            void swap_buffers(Query<std::tuple<WindowHandle>, std::tuple<>> query) {
                for (auto [window_handle] : query.iter()) {
                    if (window_handle.created) {
                        glfwSwapBuffers(window_handle.window_handle);
                    }
                }
            }

            void no_window_exists(Query<std::tuple<WindowHandle>, std::tuple<>> query,
                                  EventWriter<NoWindowExists> no_window_event) {
                for (auto [window_handle] : query.iter()) {
                    return;
                }
                spdlog::info("No window exists");
                no_window_event.write(NoWindowExists{});
            }

            void make_context_primary(Query<std::tuple<WindowHandle, PrimaryWindow>, std::tuple<>> query) {
                for (auto [window_handle] : query.iter()) {
                    if (window_handle.created) {
                        glfwMakeContextCurrent(window_handle.window_handle);
                    }
                }
            }

            void poll_events() {
                glfwPollEvents();
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }

            void exit_on_no_window(EventReader<NoWindowExists> no_window_event, EventWriter<AppExit> exit_event) {
                for (auto& _ : no_window_event.read()) {
                    glfwTerminate();
                    exit_event.write(AppExit{});
                }
            }

        }  // namespace window_gl

        class WindowGLPlugin : public Plugin {
           private:
            int primary_window_width = 480 * 3;
            int primary_window_height = 270 * 3;
            std::string primary_window_title = "Pixel Engine";

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
            void set_primary_window_title(std::string title) { primary_window_title = title; }
            void build(App& app) override {
                Node init_glfw_node, create_window_node, window_close_node, exit_on_no_window_node, insert_primary_node,
                    make_context_primary_node, primary_window_close_node;
                app.add_system_main(Startup{}, insert_primary_window, &insert_primary_node)
                    .add_system_main(Startup{}, window_gl::init_glfw, &init_glfw_node)
                    .add_system_main(Startup{}, window_gl::create_window, &start_up_window_create_node, init_glfw_node,
                                     insert_primary_node)
                    .add_system_main(PreUpdate{}, window_gl::poll_events)
                    .add_system_main(PreRender{}, window_gl::create_window, &create_window_node)
                    .add_system_main(PostRender{}, window_gl::make_context_primary, &make_context_primary_node)
                    .add_system_main(PostRender{}, window_gl::swap_buffers, &swap_buffer_node,
                                     make_context_primary_node)
                    .add_system_main(PostRender{}, window_gl::primary_window_close, &primary_window_close_node,
                                     make_context_primary_node, swap_buffer_node)
                    .add_system_main(PostRender{}, window_gl::window_close, &window_close_node, create_window_node,
                                     swap_buffer_node)
                    .add_system(PostRender{}, window_gl::no_window_exists, window_close_node, primary_window_close_node)
                    .add_system(PostUpdate{}, window_gl::exit_on_no_window);
            }
        };
    }  // namespace plugins
}  // namespace pixel_engine