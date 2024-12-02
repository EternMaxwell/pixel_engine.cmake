#include "pixel_engine/window/resources.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine::window::components;
using namespace pixel_engine::window::resources;

EPIX_API void WindowMap::insert(GLFWwindow* window, Entity entity) {
    window_map[window] = entity;
}

EPIX_API const Entity& WindowMap::get(GLFWwindow* window) const {
    return window_map.at(window);
}