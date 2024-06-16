#pragma once

#include "components.h"
#include "events.h"
#include "pixel_engine/entity.h"
#include "systems.h"

namespace pixel_engine {
    namespace window {
        using namespace components;
        using namespace systems;
        using namespace events;
        using namespace prelude;

        class WindowPlugin : public entity::Plugin {
           private:
            int primary_window_width = 480 * 3;
            int primary_window_height = 270 * 3;
            const char* primary_window_title = "Pixel Engine";

            std::function<void(Command)> insert_primary_window = [&](Command command) {
                command.spawn(
                    WindowBundle{
                        .window_handle = {.vsync = false},
                        .window_size = {.width = primary_window_width, .height = primary_window_height},
                        .window_title = {.title = primary_window_title}},
                    PrimaryWindow{});
            };

           public:
            SystemNode start_up_window_create_node;
            SystemNode swap_buffer_node;
            SystemNode create_window_node;
            void set_primary_window_size(int width, int height) {
                primary_window_width = width;
                primary_window_height = height;
            }
            void set_primary_window_title(const std::string& title) { primary_window_title = title.c_str(); }
            void build(App& app) override {
                SystemNode init_glfw_node, window_close_node, no_window_exists_node, insert_primary_node,
                    make_context_primary_node, primary_window_close_node, poll_events_node;
                app.add_system_main(Startup{}, insert_primary_window, &insert_primary_node)
                    .add_system_main(Startup{}, init_glfw, &init_glfw_node)
                    .add_system_main(
                        Startup{}, create_window, &start_up_window_create_node,
                        after(init_glfw_node, insert_primary_node))
                    .add_system_main(Startup{}, make_context_primary, after(create_window_node))
                    .add_system_main(PreUpdate{}, poll_events, &poll_events_node)
                    .add_system_main(PreUpdate{}, update_window_size, after(poll_events_node))
                    .add_system_main(PreUpdate{}, update_window_pos, after(poll_events_node))
                    .add_system_main(PreRender{}, create_window, &create_window_node)
                    .add_system_main(PostRender{}, make_context_primary, &make_context_primary_node)
                    .add_system_main(PostRender{}, swap_buffers, &swap_buffer_node, after(make_context_primary_node))
                    .add_system_main(
                        PostRender{}, primary_window_close, &primary_window_close_node,
                        after(make_context_primary_node, swap_buffer_node))
                    .add_system_main(
                        PostRender{}, window_close, &window_close_node, after(create_window_node, swap_buffer_node))
                    .add_system(
                        PostRender{}, no_window_exists, &no_window_exists_node,
                        after{window_close_node, primary_window_close_node})
                    .add_system(PostRender{}, exit_on_no_window, after{no_window_exists_node});
            }
        };
    }  // namespace window
}  // namespace pixel_engine