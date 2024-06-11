#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <fstream>

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace plugins {
        using namespace entity;
        namespace asset_server_gl {

            class AssetServerGL {
               private:
                App* m_app;
                std::string m_base_path = "./";

               public:
                AssetServerGL(App* app, std::string base_path = "./") : m_app(app), m_base_path(base_path) {}

                std::vector<char> load_shader_source(const std::string& path) {
                    std::ifstream file(m_base_path + path, std::ios::binary);
                    if (!file.is_open()) {
                        throw std::runtime_error("Failed to open file: " + path);
                    }
                    file.seekg(0, std::ios::end);
                    size_t size = file.tellg();
                    file.seekg(0, std::ios::beg);
                    std::vector<char> buffer(size);
                    file.read(buffer.data(), size);
                    return buffer;
                }

                int load_shader(const std::string& path, int type) {
                    std::vector<char> source = load_shader_source(path);
                    const char* source_ptr = source.data();
                    int shader = glCreateShader(type);
                    glShaderSource(shader, 1, &source_ptr, NULL);
                    glCompileShader(shader);
                    int success;
                    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                    if (!success) {
                        char info_log[512];
                        glGetShaderInfoLog(shader, 512, NULL, info_log);
                        throw std::runtime_error("Failed to compile shader: " + std::string(info_log));
                    }
                    return shader;
                }
            };
        }  // namespace asset_server_gl

        class AssetServerGLPlugin : public entity::Plugin {};
    }  // namespace plugins
}  // namespace pixel_engine