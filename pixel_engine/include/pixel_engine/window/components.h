#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include "pixel_engine/entity.h"

namespace pixel_engine {
namespace window {
namespace components {
/*! @brief WindowHandle component
 *  @brief Contains the GLFWwindow handle and vsync state
 */
struct WindowHandle {
    GLFWwindow* window_handle = NULL;
    bool vsync = true;
};

/*! @brief WindowCreated component
 *  @brief Indicates that the window has been created
 */
struct WindowCreated {};

/*! @brief PrimaryWindow component
 *  @brief Indicates that the window is the primary window
 */
struct PrimaryWindow {};

/*! @brief WindowSize component
 *  @brief Contains the window width and height
 */
struct WindowSize {
    int width, height;
};

/*! @brief WindowPos component
 *   @brief Contains the window x and y position
 */
struct WindowPos {
    int x, y;
};

/*! @brief WindowTitle component
 *  @brief Contains the window title
 */
struct WindowTitle {
    std::string title;
};

/*! @brief WindowHints component
 *  @brief Contains a vector of hints to be passed to GLFW
 */
struct WindowHints {
    std::vector<std::pair<int, int>> hints;
};

struct WindowBundle : entity::Bundle {
    /*! @brief WindowHandle component
     *  @brief Contains the GLFWwindow handle and vsync state
     */
    WindowHandle window_handle;
    /*! @brief WindowSize component
     *  @brief Contains the window width and height
     */
    WindowSize window_size = {480 * 3, 270 * 3};
    /*! @brief WindowPos component
     *   @brief Contains the window x and y position
     */
    WindowPos window_pos = {0, 0};
    /*! @brief WindowTitle component
     *  @brief Contains the window title
     */
    WindowTitle window_title = {"Pixel Engine"};
    /*! @brief WindowHints component
     *  @brief Contains a vector of hints to be passed to GLFW
     */
    WindowHints window_hints = {
        .hints = {
            {GLFW_CONTEXT_VERSION_MAJOR, 4},
            {GLFW_CONTEXT_VERSION_MINOR, 5},
            {GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE},
            {GLFW_VISIBLE, GLFW_FALSE}}};

    auto unpack() {
        return std::tie(
            window_handle, window_size, window_pos, window_title, window_hints);
    }
};
}  // namespace components
}  // namespace window
}  // namespace pixel_engine