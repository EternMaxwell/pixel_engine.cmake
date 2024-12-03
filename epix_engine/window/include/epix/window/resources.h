#pragma once

#include "components.h"

namespace epix {
namespace window {
namespace resources {
using namespace prelude;
using namespace components;

struct WindowThreadPool : BS::thread_pool {
    WindowThreadPool() : BS::thread_pool(1) {}
};

struct WindowMap {
   public:
    EPIX_API void insert(GLFWwindow* window, Entity entity);
    EPIX_API const Entity& get(GLFWwindow* window) const;

   protected:
    std::unordered_map<GLFWwindow*, Entity> window_map;
};
}  // namespace resources
}  // namespace window
}  // namespace epix