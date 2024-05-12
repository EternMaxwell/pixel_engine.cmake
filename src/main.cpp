#include <pixel_engine/context.h>?
#include <pixel_engine/render/renderer_gl.h>

using namespace pixel_engine;

int main() {
    context::Context* context = new context::Context();
    render::Window* window =
        (new render::Window())
            ->defaultWindowHints()
            ->setWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4)
            ->setWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5)
            ->setWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE)
            ->setWindowHint(GLFW_VISIBLE, GLFW_FALSE)
            ->createWindow(800, 600, "Pixel Engine");
    window->makeContextCurrent();
    render::OpenGLRenderer* renderer = new render::OpenGLRenderer(window);
    renderer->init();

    window->show();

    while (!window->windowShouldClose()) {
        context->pollEvents();
        renderer->beginFrame();
        renderer->endFrame();
    }

    renderer->dispose();
    delete renderer;
    delete window;
    delete context;

    return 0;
}
