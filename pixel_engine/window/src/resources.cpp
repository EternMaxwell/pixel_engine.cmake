#include "pixel_engine/window/resources.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine::window::components;
using namespace pixel_engine::window::resources;

void WindowMap::insert(GLFWwindow* window, Entity entity) {
    window_map[window] = entity;
}

const Entity& WindowMap::get(GLFWwindow* window) const {
    return window_map.at(window);
}