#pragma once

#include "components.h"
#include "events.h"
#include "pixel_engine/entity.h"
#include "systems.h"

namespace pixel_engine {
    namespace window {
        using namespace components;
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
            bool primary_window_vsync = true;
            const char* primary_window_title = "Pixel Engine";

            std::function<void(Command)> insert_primary_window = [&](Command command) {
                command.spawn(
                    WindowBundle{
                        .window_handle = {.vsync = primary_window_vsync},
                        .window_size = {.width = primary_window_width, .height = primary_window_height},
                        .window_title = {.title = primary_window_title}},
                    PrimaryWindow{});
            };

           public:
            void set_primary_window_size(int width, int height);
            void set_primary_window_title(const std::string& title);
            void set_primary_window_vsync(bool vsync);
            void build(App& app) override;
        };
    }  // namespace window
}  // namespace pixel_engine