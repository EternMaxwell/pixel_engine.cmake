#include "pixel_engine/window/resources.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine::window::components;
using namespace pixel_engine::window::resources;

void WindowMap::insert(GLFWwindow* window, Handle<Window> handle) {
    window_map[window] = handle;
}

void WindowMap::insert(GLFWwindow* window, Entity entity) {
    window_map[window] = Handle<Window>{entity};
}

const Handle<Window>& WindowMap::get(GLFWwindow* window) const {
    return window_map.at(window);
}