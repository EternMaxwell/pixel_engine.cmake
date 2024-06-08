#pragma once

#include "pixel_engine/context.h"
#include "pixel_engine/entity.h"
#include "pixel_engine/render.h"

namespace pixel_engine {
    namespace plugins {
        using namespace entity;

        namespace window_gl_systems {
            using namespace render;
            using namespace context;

            void insert_context(Command command) { command.insert_resource<Context>(); }
            void insert_window(Command command) { command.insert_resource<Window>(); }
            void init_window(Resource<Window> window) {
                if (window.has_value()) {
                    std::cout << "init_window" << std::endl;
                    auto& win = window.value();
                    win.defaultWindowHints()
                        ->setWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4)
                        ->setWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5)
                        ->setWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE)
                        ->createWindow(480 * 3, 270 * 3, "Pixel Engine");
                    std::cout << "init_window done" << std::endl;
                }
            }
            void show_window(Resource<Window> window) {
                if (window.has_value()) {
                    window.value().show();
                }
            }
            void make_context_current(Resource<Window> window) {
                if (window.has_value()) {
                    window.value().makeContextCurrent();
                }
            }
            void poll_events(Resource<Context> context) {
                if (context.has_value()) {
                    context.value().pollEvents();
                }
            }
            void swap_buffers(Resource<Window> window) {
                if (window.has_value()) {
                    window.value().swapBuffers();
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                }
            }
            void window_should_close(Resource<Window> window, EventWriter<AppExit> exit, Resource<Context> context) {
                if (window.has_value()) {
                    if (window.value().windowShouldClose()) {
                        exit.write(AppExit{});
                    }
                }
            }
        }  // namespace window_gl_systems

        class WindowGLPlugin : public Plugin {
           public:
            void build(App& app) override {
                Node insert_context_node, insert_window_node, init_window_node, show_window_node,
                    make_context_current_node, poll_events_node, window_should_close_node, swap_buffer_node;
                app.add_system_main(Startup{}, window_gl_systems::insert_context, &insert_context_node)
                    .add_system_main(Startup{}, window_gl_systems::insert_window, &insert_window_node,
                                     insert_context_node)
                    .add_system_main(Startup{}, window_gl_systems::init_window, &init_window_node, insert_window_node)
                    .add_system_main(Startup{}, window_gl_systems::show_window, &show_window_node, init_window_node)
                    .add_system_main(Startup{}, window_gl_systems::make_context_current, &make_context_current_node,
                                     init_window_node)
                    .add_system_main(Update{}, window_gl_systems::poll_events, &poll_events_node)
                    .add_system_main(Update{}, window_gl_systems::swap_buffers, &swap_buffer_node, poll_events_node)
                    .add_system_main(Update{}, window_gl_systems::window_should_close, &window_should_close_node,
                                     swap_buffer_node, poll_events_node);
            }
        };
    }  // namespace plugins
}  // namespace pixel_engine