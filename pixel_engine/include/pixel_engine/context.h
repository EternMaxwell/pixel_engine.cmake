#ifndef PIXEL_ENGINE_CONTEXT_H_
#define PIXEL_ENGINE_CONTEXT_H_

#include <GLFW/glfw3.h>

#include <iostream>

namespace pixel_engine {
    namespace context {

        void error_callback(int error, const char* description) {
			std::cerr << "Error: " << description << std::endl;
		}

        class Context {
           public:
            Context() {
                if (!glfwInit()) {
                    throw std::runtime_error("Failed to initialize GLFW");
                }
                glfwSetErrorCallback(error_callback);
            }

            void pollEvents() { glfwPollEvents(); }

            void terminate() { glfwTerminate(); }
            ~Context() { glfwTerminate(); }
        };

    }  // namespace context
}  // namespace pixel_engine

#endif  // PIXEL_ENGINE_CONTEXT_H_