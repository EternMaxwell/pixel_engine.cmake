#include <GLFW/glfw3.h>
#include <gl/GL.h>

#include "pixel_engine/render.h"

namespace pixel_engine {
    namespace render {
        class OpenGLRenderer : Renderer {
           private:
            Window* window;

           public:
            OpenGLRenderer(Window* window) : window(window) {}

            void init() {}

            void beginFrame() {}

            void endFrame() {
                glfwSwapBuffers(window->getGLFWWindow());
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }

            void dispose() {}

            ~OpenGLRenderer() {}
        };
    }  // namespace render
}  // namespace pixel_engine