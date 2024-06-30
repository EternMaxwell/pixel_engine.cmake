﻿#include "pixel_engine/asset_server_gl/asset_server_gl.h"

#include "pixel_engine/asset_server_gl/resources.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

std::vector<char> pixel_engine::asset_server_gl::resources::AssetServerGL::load_shader_source(const std::string& path) {
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

int pixel_engine::asset_server_gl::resources::AssetServerGL::loat_shader(const std::vector<char>& source, int type) {
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

int pixel_engine::asset_server_gl::resources::AssetServerGL::load_shader(const std::string& path, int type) {
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

int pixel_engine::asset_server_gl::resources::AssetServerGL::load_image_2d(const std::string& path) {
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

int pixel_engine::asset_server_gl::resources::AssetServerGL::create_sampler(
    int wrap_s, int wrap_t, int min_filter, int mag_filter) {
    unsigned int sampler;
    glCreateSamplers(1, &sampler);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, wrap_s);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, wrap_t);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, min_filter);
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, mag_filter);
    return sampler;
}

FT_Face pixel_engine::asset_server_gl::resources::AssetServerGL::load_font(
    const FT_Library& library, const std::string& path) {
    if (m_font_faces.find(path) != m_font_faces.end()) {
        return m_font_faces[path];
    }
    FT_Face face;
    FT_Error error = FT_New_Face(library, (m_base_path + path).c_str(), 0, &face);
    if (error) {
        throw std::runtime_error("Failed to load font: " + path);
    }
    m_font_faces[path] = face;
    return face;
}

void pixel_engine::asset_server_gl::AssetServerGLPlugin::build(App& app) {
    app.configure_sets(AssetServerGLSets::insert_asset_server, AssetServerGLSets::after_insertion)
        .add_system(PreStartup{}, insert_asset_server, in_set(AssetServerGLSets::insert_asset_server));
}
