#pragma once

#include <glad/glad.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace plugins {
        using namespace entity;
        namespace asset_server_gl {

            class AssetServerGL {
               private:
                std::string m_base_path = "./";

               public:
                AssetServerGL(std::string base_path = "./") : m_base_path(base_path) {}

                std::vector<char> load_shader_source(const std::string& path) {
                    std::ifstream file(m_base_path + path);
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
                        throw std::runtime_error("Failed to link program: " + std::string(info_log));
                    }
                    return shader;
                }

                int load_image_2d(const std::string& path) {
                    stbi_set_flip_vertically_on_load(true);
                    int width, height, channels;
                    stbi_uc* data = stbi_load((m_base_path + path).c_str(), &width, &height, &channels, 0);
                    if (!data) {
                        throw std::runtime_error("Failed to load image: " + path);
                    }
                    unsigned int texture;
                    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
                    glTextureStorage2D(texture, 1, GL_RGBA8, width, height);
                    if (channels == 3) {
                        glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
                    } else if (channels == 4) {
                        glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
                    }
                    glGenerateMipmap(GL_TEXTURE_2D);
                    stbi_image_free(data);
                    return texture;
                }

                int create_sampler(int wrap_s, int wrap_t, int min_filter, int mag_filter) {
                    unsigned int sampler;
                    glCreateSamplers(1, &sampler);
                    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, wrap_s);
                    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, wrap_t);
                    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, min_filter);
                    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, mag_filter);
                    return sampler;
                }
            };

            void insert_asset_server(Command command) { command.insert_resource(AssetServerGL{}); }
        }  // namespace asset_server_gl

        class AssetServerGLPlugin : public entity::Plugin {
           public:
            Node insert_asset_server_node;

            void build(App& app) override {
                using namespace asset_server_gl;
                app.add_system(Startup{}, insert_asset_server, &insert_asset_server_node);
            }
        };
    }  // namespace plugins
}  // namespace pixel_engine