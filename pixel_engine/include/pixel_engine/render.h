#include <GLFW/glfw3.h>

#include <iostream>

namespace pixel_engine {
    namespace render {
        class Window {
           private:
            GLFWwindow* window;

           public:
            Window(int width, int height, const std::string& title) {
                window = glfwCreateWindow(width, height, title.c_str(), nullptr,
                                          nullptr);
                if (!window) {
                    glfwTerminate();
                    throw std::runtime_error("Failed to create GLFW window");
                }

                glfwSetWindowSizeCallback(
                    window, [](GLFWwindow* window, int width, int height) {
                        glViewport(0, 0, width, height);
                    });
            }

            Window() {}

            GLFWwindow* getGLFWWindow() { return window; }

            Window* defaultWindowHints() {
                glfwDefaultWindowHints();
                return this;
            }

            Window* setWindowHint(int hint, int value) {
                glfwWindowHint(hint, value);
                return this;
            }

            Window* createWindow(int width, int height,
                                 const std::string& title) {
                window = glfwCreateWindow(width, height, title.c_str(), nullptr,
                                          nullptr);
                if (!window) {
                    glfwTerminate();
                    throw std::runtime_error("Failed to create GLFW window");
                }

                glfwSetWindowSizeCallback(
                    window, [](GLFWwindow* window, int width, int height) {
                        glViewport(0, 0, width, height);
                    });

                return this;
            }

            void getSize(int* width, int* height) {
                glfwGetWindowSize(window, width, height);
            }

            void setShouldClose(bool value) {
                glfwSetWindowShouldClose(window, value);
            }

            void setTitle(const std::string& title) {
                glfwSetWindowTitle(window, title.c_str());
            }

            void makeContextCurrent() { glfwMakeContextCurrent(window); }

            void setSize(int width, int height) {
                glfwSetWindowSize(window, width, height);
            }

            void setPos(int x, int y) { glfwSetWindowPos(window, x, y); }

            void setSizeLimits(int minWidth, int minHeight, int maxWidth,
                               int maxHeight) {
                glfwSetWindowSizeLimits(window, minWidth, minHeight, maxWidth,
                                        maxHeight);
            }

            void setIcon(const GLFWimage* images) {
                glfwSetWindowIcon(window, 1, images);
            }

            void setOpacity(float opacity) {
                glfwSetWindowOpacity(window, opacity);
            }

            void setResizable(bool value) {
                glfwSetWindowAttrib(window, GLFW_RESIZABLE, value);
            }

            void setDecorated(bool value) {
                glfwSetWindowAttrib(window, GLFW_DECORATED, value);
            }

            void setFloating(bool value) {
                glfwSetWindowAttrib(window, GLFW_FLOATING, value);
            }

            void setFocusOnShow(bool value) {
                glfwSetWindowAttrib(window, GLFW_FOCUS_ON_SHOW, value);
            }

            bool windowShouldClose() { return glfwWindowShouldClose(window); }

            void show() { glfwShowWindow(window); }

            void swapBuffers() { glfwSwapBuffers(window); }

            ~Window() {
                if (window) glfwDestroyWindow(window);
            }
        };

        class Renderer {
           public:
            virtual void init() = 0;
            virtual void dispose() = 0;
            virtual void beginFrame() = 0;
            virtual void endFrame() = 0;
        };
    }  // namespace render
}  // namespace pixel_engine