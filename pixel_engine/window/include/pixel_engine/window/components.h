#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#include <pixel_engine/app.h>

namespace pixel_engine {
namespace window {
namespace components {
using namespace prelude;

struct PrimaryWindow {};

struct Window;

struct WindowDescription {
    std::string title = "Pixel Engine";
    int width = 480 * 3, height = 270 * 3;
    bool vsync = true;
    std::vector<std::pair<int, int>> hints;

   public:
    WindowDescription& set_title(std::string title);
    WindowDescription& set_size(int width, int height);
    WindowDescription& set_vsync(bool vsync);
    WindowDescription& add_hint(int hint, int value);
    WindowDescription& set_hints(std::vector<std::pair<int, int>> hints);
};

struct Window {
    struct extent {
        int width;
        int height;
    };

    struct ivec2 {
        int x;
        int y;
    };

    struct dvec2 {
        double x;
        double y;
    };

    GLFWwindow* m_handle;
    std::string m_title;
    bool m_vsync;
    extent m_size;
    ivec2 m_pos;
    dvec2 m_cursor_pos;
    std::optional<dvec2> m_cursor_move;

   public:
    const dvec2& get_cursor() const;
    const std::optional<dvec2>& get_cursor_move() const;
    const ivec2& get_pos() const;
    const extent& get_size() const;
    GLFWwindow* get_handle() const;
    void show();
    void hide();
    bool vsync() const;
    bool should_close() const;
    void destroy();
    void set_cursor(double x, double y);
    bool focused() const;
    bool is_fullscreen() const;
    void set_fullscreen();
    void fullscreen_off();
    void toggle_fullscreen();
};

std::optional<Window> create_window(const WindowDescription& desc);
void update_cursor(Window& window);
void update_size(Window& window);
void update_pos(Window& window);
}  // namespace components
}  // namespace window
}  // namespace pixel_engine