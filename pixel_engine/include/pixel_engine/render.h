#ifndef PIXEL_ENGINE_RENDER_H_
#define PIXEL_ENGINE_RENDER_H_

#include <GLFW/glfw3.h>

#include <iostream>

namespace pixel_engine {
    namespace render {
        class Window {
           private:
            GLFWwindow* window;
            int preservedWidth, preservedHeight;
            int preservedPosX, preservedPosY;

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

            bool isFocused() {
                return glfwGetWindowAttrib(window, GLFW_FOCUSED);
            }

            void getSize(int* width, int* height) {
                glfwGetWindowSize(window, width, height);
            }

            void getPos(int* x, int* y) { glfwGetWindowPos(window, x, y); }

            double getRatio() {
                int width, height;
                getSize(&width, &height);
                return (double)width / (double)height;
            }

            void setShouldClose(bool value) {
                glfwSetWindowShouldClose(window, value);
            }

            void setFullscreen(bool value) {
                GLFWmonitor* monitor = glfwGetWindowMonitor(window);
                if (value && monitor == NULL) {
                    // preserve pos and size
                    glfwGetWindowSize(window, &preservedWidth,
                                      &preservedHeight);
                    glfwGetWindowPos(window, &preservedPosX, &preservedPosY);

                    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width,
                                         mode->height, mode->refreshRate);
                } else if (monitor != NULL) {
                    glfwSetWindowMonitor(window, nullptr, preservedPosX,
                                         preservedPosY, preservedWidth,
                                         preservedHeight, 0);
                }
            }

            bool isWindowed() { return glfwGetWindowMonitor(window) == NULL; }

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

            void setFrameRate(int fps) {
                glfwSetWindowAttrib(window, GLFW_REFRESH_RATE, fps);
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

        class Pipeline {};
    }  // namespace render
}  // namespace pixel_engine

#endif  // PIXEL_ENGINE_RENDER_H_