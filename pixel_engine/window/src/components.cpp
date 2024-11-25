#include "pixel_engine/window/components.h"
#include "pixel_engine/window/resources.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine::window;
using namespace pixel_engine::window::components;
using namespace pixel_engine::window::resources;

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
const std::optional<Window::dvec2>& Window::get_cursor_move() const {
    return m_cursor_move;
}
const Window::ivec2& Window::get_pos() const {
    return m_pos;
}
const Window::extent& Window::get_size() const {
    return m_size;
}
GLFWwindow* Window::get_handle() const { return m_handle; }
void Window::context_current() { glfwMakeContextCurrent(m_handle); }
void Window::detach_context() {
    if (glfwGetCurrentContext() == m_handle) glfwMakeContextCurrent(nullptr);
}
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
    auto monitor   = glfwGetPrimaryMonitor();
    auto mode      = glfwGetVideoMode(monitor);
    auto* user_ptr = static_cast<std::pair<Entity, WindowThreadPool*>*>(
        glfwGetWindowUserPointer(m_handle)
    );
    auto&& [entity, pool] = *user_ptr;
    auto ftr              = pool->submit_task([=]() {
        glfwSetWindowMonitor(
            m_handle, monitor, 0, 0, mode->width, mode->height,
            mode->refreshRate
        );
    });
    ftr.wait();
    m_size_cache = m_size;
    m_pos_cache  = m_pos;
    m_size       = {mode->width, mode->height};
    m_pos        = {0, 0};
}
void Window::fullscreen_off() {
    auto mode      = glfwGetVideoMode(glfwGetPrimaryMonitor());
    auto* user_ptr = static_cast<std::pair<Entity, WindowThreadPool*>*>(
        glfwGetWindowUserPointer(m_handle)
    );
    auto&& [entity, pool] = *user_ptr;
    m_size = m_size_cache;
    m_pos  = m_pos_cache;
    auto ftr              = pool->submit_task([=]() {
        glfwSetWindowMonitor(
            m_handle, nullptr, m_pos.x, m_pos.y, m_size.width, m_size.height, 0
        );
    });
    ftr.wait();
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
    return Window{
        window, desc.title, desc.vsync, {desc.width, desc.height}, {x, y}
    };
}
void components::update_cursor(Window& window) {
    double x, y;
    glfwGetCursorPos(window.m_handle, &x, &y);
    if (x != window.m_cursor_pos.x || y != window.m_cursor_pos.y) {
        window.m_cursor_move = {
            x - window.m_cursor_pos.x, y - window.m_cursor_pos.y
        };
    } else {
        window.m_cursor_move = std::nullopt;
    }
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