#include "pixel_engine/render_gl/components.h"
#include "pixel_engine/render_gl/render_gl.h"
#include "pixel_engine/render_gl/systems.h"

using namespace pixel_engine::render_gl::systems;
using namespace pixel_engine::render_gl::components;
using namespace pipeline;

void VertexArray::create() { glCreateVertexArrays(1, &id); }

bool VertexArray::valid() const { return id != 0; }

void pipeline::Buffer::create() { glCreateBuffers(1, &id); }

bool pipeline::Buffer::valid() const { return id != 0; }

void Texture::create(int type) { glCreateTextures(type, 1, &id); }

bool Texture::valid() const { return id != 0; }

void Sampler::create() { glCreateSamplers(1, &id); }

bool Sampler::valid() const { return id != 0; }

pipeline::Buffer& UniformBufferBindings::operator[](size_t index) { return buffers[index]; }

pipeline::Buffer& StorageBufferBindings::operator[](size_t index) { return buffers[index]; }

pipeline::Image& TextureBindings::operator[](size_t index) { return textures[index]; }

pipeline::Texture& ImageTextureBindings::operator[](size_t index) { return images[index]; }

void pixel_engine::render_gl::components::Buffer::write(const void* rdata, size_t size, size_t offset) {
    if (offset + size > data.size()) {
        data.resize(offset + size);
    }
    memcpy(data.data() + offset, rdata, size);
}

void pixel_engine::render_gl::RenderGLPlugin::build(App& app) {
    using namespace render_gl;
    using namespace window;
    app.configure_sets(RenderGLStartupSets::context_creation, RenderGLStartupSets::after_context_creation)
        .add_system_main(
            PreStartup{}, context_creation,
            in_set(window::WindowStartUpSets::after_window_creation, RenderGLStartupSets::context_creation))
        .configure_sets(RenderGLPreRenderSets::create_pipelines, RenderGLPreRenderSets::after_create_pipelines)
        .add_system_main(PreRender{}, create_pipelines, in_set(RenderGLPreRenderSets::create_pipelines))
        .add_system_main(PreRender{}, clear_color)
        .add_system_main(PreRender{}, update_viewport);
}

void pixel_engine::render_gl::systems::clear_color(Query<Get<WindowHandle>, With<WindowCreated>, Without<>> query) {
    for (auto [window_handle] : query.iter()) {
        if (glfwGetCurrentContext() != window_handle.window_handle) glfwMakeContextCurrent(window_handle.window_handle);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void pixel_engine::render_gl::systems::update_viewport(
    Query<Get<WindowHandle, WindowSize>, With<WindowCreated>, Without<>> query) {
    for (auto [window_handle, window_size] : query.iter()) {
        if (glfwGetCurrentContext() != window_handle.window_handle) glfwMakeContextCurrent(window_handle.window_handle);
        glViewport(0, 0, window_size.width, window_size.height);
    }
}

void pixel_engine::render_gl::systems::context_creation(
    Query<Get<WindowHandle>, With<WindowCreated>, Without<>> query) {
    for (auto [window_handle] : query.iter()) {
        if (glfwGetCurrentContext() != window_handle.window_handle) glfwMakeContextCurrent(window_handle.window_handle);
        gladLoadGL();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
    }
}

void pixel_engine::render_gl::systems::create_pipelines(
    Command command, Query<Get<entt::entity, Pipeline, Shaders, Attribs>, With<>, Without<ProgramLinked>> query) {
    for (auto [id, pipeline, shaders, attribs] : query.iter()) {
        spdlog::debug("Creating new pipeline");
        pipeline.program = glCreateProgram();
        glCreateBuffers(1, &pipeline.vertex_buffer.buffer);
        glBindBuffer(GL_ARRAY_BUFFER, pipeline.vertex_buffer.buffer);
        glCreateVertexArrays(1, &pipeline.vertex_array);
        glBindVertexArray(pipeline.vertex_array);
        if (shaders.vertex_shader) {
            glAttachShader(pipeline.program, shaders.vertex_shader);
        }
        if (shaders.fragment_shader) {
            glAttachShader(pipeline.program, shaders.fragment_shader);
        }
        if (shaders.geometry_shader) {
            glAttachShader(pipeline.program, shaders.geometry_shader);
        }
        if (shaders.tess_control_shader) {
            glAttachShader(pipeline.program, shaders.tess_control_shader);
        }
        if (shaders.tess_evaluation_shader) {
            glAttachShader(pipeline.program, shaders.tess_evaluation_shader);
        }
        for (auto& attrib : attribs.attribs) {
            glEnableVertexAttribArray(attrib.location);
            glVertexAttribPointer(
                attrib.location, attrib.size, attrib.type, attrib.normalized, attrib.stride, (void*)attrib.offset);
        }
        glLinkProgram(pipeline.program);
        int success;
        glGetProgramiv(pipeline.program, GL_LINK_STATUS, &success);
        if (!success) {
            char info_log[512];
            glGetProgramInfoLog(pipeline.program, 512, NULL, info_log);
            throw std::runtime_error("Failed to link program: " + std::string(info_log));
        }
        command.entity(id).emplace(ProgramLinked{});
        spdlog::debug("End creating new pipeline.");
    }
}
