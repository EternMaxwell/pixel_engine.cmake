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

        enum class WindowStartUpSets {
            glfw_initialization,
            window_creation,
            after_window_creation,
        };

        enum class WindowPreUpdateSets {
            poll_events,
            update_window_data,
        };

        enum class WindowPreRenderSets {
            before_create,
            window_creation,
            after_create,
        };

        enum class WindowPostRenderSets {
            before_swap_buffers,
            swap_buffers,
            window_close,
            after_close_window,
        };

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
            void set_primary_window_size(int width, int height) {
                primary_window_width = width;
                primary_window_height = height;
            }
            void set_primary_window_title(const std::string& title) { primary_window_title = title.c_str(); }
            void build(App& app) override {
                SystemNode no_window_exists_node, insert_primary_node;
                app.configure_sets(
                       WindowStartUpSets::glfw_initialization, WindowStartUpSets::window_creation,
                       WindowStartUpSets::after_window_creation)
                    .add_system_main(Startup{}, insert_primary_window, &insert_primary_node)
                    .add_system_main(Startup{}, init_glfw, in_set(WindowStartUpSets::glfw_initialization))
                    .add_system_main(
                        Startup{}, create_window, after(insert_primary_node),
                        in_set(WindowStartUpSets::window_creation))
                    .configure_sets(WindowPreUpdateSets::poll_events, WindowPreUpdateSets::update_window_data)
                    .add_system_main(PreUpdate{}, poll_events, in_set(WindowPreUpdateSets::poll_events))
                    .add_system_main(PreUpdate{}, update_window_size, in_set(WindowPreUpdateSets::update_window_data))
                    .add_system_main(PreUpdate{}, update_window_pos, in_set(WindowPreUpdateSets::update_window_data))
                    .configure_sets(
                        WindowPreRenderSets::before_create, WindowPreRenderSets::window_creation,
                        WindowPreRenderSets::after_create)
                    .add_system_main(PreRender{}, create_window, in_set(WindowPreRenderSets::window_creation))
                    .configure_sets(
                        WindowPostRenderSets::before_swap_buffers, WindowPostRenderSets::swap_buffers,
                        WindowPostRenderSets::window_close, WindowPostRenderSets::after_close_window)
                    .add_system_main(PostRender{}, swap_buffers, in_set(WindowPostRenderSets::swap_buffers))
                    .add_system_main(PostRender{}, primary_window_close, in_set(WindowPostRenderSets::window_close))
                    .add_system_main(PostRender{}, window_close, in_set(WindowPostRenderSets::window_close))
                    .add_system(
                        PostRender{}, no_window_exists, &no_window_exists_node,
                        in_set(WindowPostRenderSets::after_close_window))
                    .add_system(
                        PostRender{}, exit_on_no_window, after(no_window_exists_node),
                        in_set(WindowPostRenderSets::after_close_window));
            }
        };
    }  // namespace window
}  // namespace pixel_engine