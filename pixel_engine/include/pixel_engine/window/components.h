#pragma once

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace window {
        namespace components {
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
                entity::Bundle bundle;
                WindowHandle window_handle;
                WindowSize window_size = {480 * 3, 270 * 3};
                WindowPos window_pos = {0, 0};
                WindowTitle window_title = {"Pixel Engine"};
                WindowHints window_hints = {
                    .hints = {
                        {GLFW_CONTEXT_VERSION_MAJOR, 4},
                        {GLFW_CONTEXT_VERSION_MINOR, 5},
                        {GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE},
                        {GLFW_VISIBLE, GLFW_FALSE}}};
            };
        }  // namespace components
    }      // namespace window
}  // namespace pixel_engine