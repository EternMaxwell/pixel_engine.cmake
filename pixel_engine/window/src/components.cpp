#include "pixel_engine/window/components.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine::window;
using namespace pixel_engine::window::components;

WindowDescription& WindowDescription::set_title(std::string title) {
    this->title = title;
    return *this;
}
WindowDescription& WindowDescription::set_size(int width, int height) {
    this->width  = width;
    this->height = height;
    return *this;
}
WindowDescription& WindowDescription::set_vsync(bool vsync) {
    this->vsync = vsync;
    return *this;
}
WindowDescription& WindowDescription::add_hint(int hint, int value) {
    hints.push_back({hint, value});
    return *this;
}
WindowDescription& WindowDescription::set_hints(
    std::vector<std::pair<int, int>> hints
) {
    this->hints = hints;
    return *this;
}

const Window::dvec2& Window::get_cursor() const { return m_cursor_pos; }
const Window::ivec2& Window::get_pos() const { return m_pos; }
const Window::extent& Window::get_size() const { return m_size; }
GLFWwindow* Window::get_handle() const { return m_handle; }
void Window::show() { glfwShowWindow(m_handle); }
void Window::hide() { glfwHideWindow(m_handle); }
bool Window::vsync() const { return m_vsync; }
bool Window::should_close() const {
    if (!m_handle) return true;
    return glfwWindowShouldClose(m_handle);
}
void Window::destroy() {
    glfwDestroyWindow(m_handle);
    m_handle = nullptr;
}
void Window::set_cursor(double x, double y) {
    glfwSetCursorPos(m_handle, x, y);
    m_cursor_pos = dvec2{x, y};
}
bool Window::focused() const {
    return glfwGetWindowAttrib(m_handle, GLFW_FOCUSED);
}
bool Window::is_fullscreen() const {
    return glfwGetWindowMonitor(m_handle) != nullptr;
}
void Window::set_fullscreen() {
    auto monitor = glfwGetPrimaryMonitor();
    auto mode    = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(
        m_handle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate
    );
}
void Window::fullscreen_off() {
    auto mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowMonitor(
        m_handle, nullptr, m_pos.x, m_pos.y, m_size.width, m_size.height,
        mode->refreshRate
    );
}
void Window::toggle_fullscreen() {
    if (is_fullscreen()) {
        fullscreen_off();
    } else {
        set_fullscreen();
    }
}

std::optional<Window> components::create_window(const WindowDescription& desc) {
    glfwDefaultWindowHints();
    for (auto [hint, value] : desc.hints) {
        glfwWindowHint(hint, value);
    }
    GLFWwindow* window = glfwCreateWindow(
        desc.width, desc.height, desc.title.c_str(), nullptr, nullptr
    );
    if (!window) {
        return std::nullopt;
    }
    int x, y;
    glfwGetWindowPos(window, &x, &y);
    return Window{window, desc.vsync, {desc.width, desc.height}, {x, y}};
}
void components::update_cursor(Window& window) {
    double x, y;
    glfwGetCursorPos(window.m_handle, &x, &y);
    window.m_cursor_pos = {x, y};
}
void components::update_size(Window& window) {
    if (window.is_fullscreen()) return;
    int width, height;
    glfwGetWindowSize(window.m_handle, &width, &height);
    window.m_size = {width, height};
}
void components::update_pos(Window& window) {
    if (window.is_fullscreen()) return;
    int x, y;
    glfwGetWindowPos(window.m_handle, &x, &y);
    window.m_pos = {x, y};
}