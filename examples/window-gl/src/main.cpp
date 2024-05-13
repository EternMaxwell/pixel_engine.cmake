#include <pixel_engine/context.h>
#include <pixel_engine/input.h>
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
            ->createWindow(480 * 3, 270 * 3, "Pixel Engine");
    window->makeContextCurrent();
    render::OpenGLRenderer* renderer = new render::OpenGLRenderer(window);
    renderer->init();

    input::InputTool inputTool(window);

    window->show();

    while (!window->windowShouldClose()) {
        context->pollEvents();
        inputTool.input();

        double scrollx, scrolly;
        inputTool.getScrollOffset(&scrollx, &scrolly);
        std::cout << "Scroll offset: " << scrollx << ", " << scrolly
                  << std::endl;

        double posx, posy;
        inputTool.getMousePos(&posx, &posy);
        std::cout << "Mouse pos: " << posx << ", " << posy << std::endl;
        std::cout << "Window focused: "
                  << (window->isFocused() ? "true" : "false") << std::endl;

        renderer->beginFrame();
        renderer->endFrame();
    }

    renderer->dispose();
    delete renderer;
    delete window;
    delete context;

    return 0;
}
