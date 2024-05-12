#include <GLFW/glfw3.h>

#include <map>
#include <vector>

#include "pixel_engine/render.h"

namespace pixel_engine {
    namespace input {
        static const int STATE_RELEASED = 0;
        static const int STATE_PRESSED = 1;
        static const int STATE_JUST_PRESSED = 2;
        static const int STATE_JUST_RELEASED = 3;

        static double scrollOffset[2];
        static bool scrollOffsetChanged;

        class InputTool {
           public:
            InputTool(pixel_engine::render::Window* window) : window(window) {
                keysJustPressed = std::vector<int>();
                keyStates = std::map<int, int>();
                mouseButtonStates = std::map<int, int>();
                mouseButtonsJustReleased = std::vector<int>();
                mousePos[0] = 0;
                mousePos[1] = 0;
                mousePosPrev[0] = 0;
                mousePosPrev[1] = 0;
                scrollOffset[0] = 0;
                scrollOffset[1] = 0;
                scrollOffsetChanged = false;

                glfwSetScrollCallback(
                    window->getGLFWWindow(),
                    [](GLFWwindow* window, double xoffset, double yoffset) {
                        scrollOffset[0] = xoffset;
                        scrollOffset[1] = yoffset;
                        scrollOffsetChanged = true;
                    });
            }

            /*! @brief Updates the input state.
             */
            void input() {
                if (scrollOffsetChanged) {
                    scrollOffsetChanged = false;
                } else {
                    scrollOffset[0] = 0;
                    scrollOffset[1] = 0;
                }
                keysJustPressed.clear();
                mouseButtonsJustReleased.clear();
                for (auto& keyState : keyStates) {
                    if (keyState.second == STATE_JUST_PRESSED) {
                        keyState.second = STATE_PRESSED;
                    } else if (keyState.second == STATE_JUST_RELEASED) {
                        keyState.second = STATE_RELEASED;
                    }
                    if (glfwGetKey(window->getGLFWWindow(), keyState.first) ==
                            GLFW_PRESS &&
                        keyState.second == STATE_RELEASED) {
                        keyState.second = STATE_JUST_PRESSED;
                    } else if (glfwGetKey(window->getGLFWWindow(),
                                          keyState.first) == GLFW_RELEASE &&
                               keyState.second == STATE_PRESSED) {
                        keyState.second = STATE_JUST_RELEASED;
                    }
                }
                for (auto& mouseButtonState : mouseButtonStates) {
                    if (mouseButtonState.second == STATE_JUST_PRESSED) {
                        mouseButtonState.second = STATE_PRESSED;
                    } else if (mouseButtonState.second == STATE_JUST_RELEASED) {
                        mouseButtonState.second = STATE_RELEASED;
                    }
                    if (glfwGetMouseButton(window->getGLFWWindow(),
                                           mouseButtonState.first) ==
                            GLFW_PRESS &&
                        mouseButtonState.second == STATE_RELEASED) {
                        mouseButtonState.second = STATE_JUST_PRESSED;
                    } else if (glfwGetMouseButton(window->getGLFWWindow(),
                                                  mouseButtonState.first) ==
                                   GLFW_RELEASE &&
                               mouseButtonState.second == STATE_PRESSED) {
                        mouseButtonState.second = STATE_JUST_RELEASED;
                    }
                }
                mousePosPrev[0] = mousePos[0];
                mousePosPrev[1] = mousePos[1];
                glfwGetCursorPos(window->getGLFWWindow(), &mousePos[0],
                                 &mousePos[1]);
            }

            /*! @brief Get if key is currently pressed
             * @param key The key to check
             * @return If the key is currently pressed
             */
            bool isKeyPressed(int key) {
                if (keyStates.find(key) == keyStates.end()) {
                    keyStates[key] = STATE_RELEASED;
                }
                return keyStates[key] == STATE_PRESSED ||
                       keyStates[key] == STATE_JUST_PRESSED;
            }

            /*! @brief Get if key is currently released
             * @param key The key to check
             * @return If the key is currently released
             */
            bool isKeyReleased(int key) {
                if (keyStates.find(key) == keyStates.end()) {
                    keyStates[key] = STATE_RELEASED;
                }
                return keyStates[key] == STATE_RELEASED ||
                       keyStates[key] == STATE_JUST_RELEASED;
            }

            /*! @brief Get if key was just pressed
             * @param key The key to check
             * @return If the key was just pressed
             */
            bool isKeyJustPressed(int key) {
                if (keyStates.find(key) == keyStates.end()) {
                    keyStates[key] = STATE_RELEASED;
                }
                return keyStates[key] == STATE_JUST_PRESSED;
            }

            /*! @brief Get if key was just released
             * @param key The key to check
             * @return If the key was just released
             */
            bool isKeyJustReleased(int key) {
                if (keyStates.find(key) == keyStates.end()) {
                    keyStates[key] = STATE_RELEASED;
                }
                return keyStates[key] == STATE_JUST_RELEASED;
            }

            /*! @brief Get if mouse button is currently pressed
             * @param button The button to check
             * @return If the button is currently pressed
             */
            bool isMouseButtonPressed(int button) {
                if (mouseButtonStates.find(button) == mouseButtonStates.end()) {
                    mouseButtonStates[button] = STATE_RELEASED;
                }
                return mouseButtonStates[button] == STATE_PRESSED ||
                       mouseButtonStates[button] == STATE_JUST_PRESSED;
            }

            /*! @brief Get if mouse button is currently released
             * @param button The button to check
             * @return If the button is currently released
             */
            bool isMouseButtonReleased(int button) {
                if (mouseButtonStates.find(button) == mouseButtonStates.end()) {
                    mouseButtonStates[button] = STATE_RELEASED;
                }
                return mouseButtonStates[button] == STATE_RELEASED ||
                       mouseButtonStates[button] == STATE_JUST_RELEASED;
            }

            /*! @brief Get if mouse button was just pressed
             * @param button The button to check
             * @return If the button was just pressed
             */
            bool isMouseButtonJustPressed(int button) {
                if (mouseButtonStates.find(button) == mouseButtonStates.end()) {
                    mouseButtonStates[button] = STATE_RELEASED;
                }
                return mouseButtonStates[button] == STATE_JUST_PRESSED;
            }

            /*! @brief Get if mouse button was just released
             * @param button The button to check
             * @return If the button was just released
             */
            bool isMouseButtonJustReleased(int button) {
                if (mouseButtonStates.find(button) == mouseButtonStates.end()) {
                    mouseButtonStates[button] = STATE_RELEASED;
                }
                return mouseButtonStates[button] == STATE_JUST_RELEASED;
            }

            /*! @brief Get the current mouse position
             * @return The current mouse position
             */
            void getMousePos(double* x, double* y) {
                *x = mousePos[0];
                *y = mousePos[1];
            }

            /*! @brief Get the previous mouse position
             * @return The previous mouse position
             */
            void getMousePosPrev(double* x, double* y) {
                *x = mousePosPrev[0];
                *y = mousePosPrev[1];
            }

            /*! @brief Get the scroll offset
             * @return The scroll offset
             */
            void getScrollOffset(double* x, double* y) {
                *x = scrollOffset[0];
                *y = scrollOffset[1];
            }

            ~InputTool() {}

           private:
            pixel_engine::render::Window* window;
            std::map<int, int> keyStates;
            std::map<int, int> mouseButtonStates;
            std::vector<int> keysJustPressed;
            std::vector<int> mouseButtonsJustReleased;
            double mousePos[2];
            double mousePosPrev[2];
        };
    }  // namespace input
}  // namespace pixel_engine