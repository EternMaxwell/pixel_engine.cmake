#pragma once

#include "components.h"

namespace pixel_engine {
namespace window {
namespace resources {
using namespace prelude;
using namespace components;

struct WindowMap {
   public:
    void insert(GLFWwindow* window, Handle<Window> handle);
    void insert(GLFWwindow* window, Entity entity);
    const Handle<Window>& get(GLFWwindow* window) const;

   protected:
    std::unordered_map<GLFWwindow*, Handle<Window>> window_map;
};
}  // namespace resources
}  // namespace window
}  // namespace pixel_engine