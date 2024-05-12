#ifndef PIXEL_ENGINE_CONTEXT_H_
#define PIXEL_ENGINE_CONTEXT_H_

#include <GLFW/glfw3.h>

#include <iostream>

namespace pixel_engine {
    namespace context {
        class Context {
           public:
            Context() {
                if (!glfwInit()) {
#ifndef NDEBUG
                    throw std::runtime_error("Failed to initialize GLFW");
#endif
                }
            }

            void pollEvents() { glfwPollEvents(); }

            ~Context() { glfwTerminate(); }
        };

    }  // namespace context
}  // namespace pixel_engine

#endif  // PIXEL_ENGINE_CONTEXT_H_