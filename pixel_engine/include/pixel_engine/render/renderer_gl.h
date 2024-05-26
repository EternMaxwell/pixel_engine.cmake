#ifndef PIXEL_ENGINE_RENDER_RENDERER_GL_H_
#define PIXEL_ENGINE_RENDER_RENDERER_GL_H_

#include <GLFW/glfw3.h>
#include <gl/GL.h>

#include "pixel_engine/render.h"

namespace pixel_engine {
    namespace render {
        class OpenGLRenderer : pixel_engine::render::Renderer {
           private:
            pixel_engine::render::Window* window;

           public:
            OpenGLRenderer(pixel_engine::render::Window* window)
                : window(window) {}

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

#endif  // PIXEL_ENGINE_RENDER_RENDERER_GL_H_