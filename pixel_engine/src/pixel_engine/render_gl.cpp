﻿#include "pixel_engine/render_gl/components.h"
#include "pixel_engine/render_gl/render_gl.h"
#include "pixel_engine/render_gl/systems.h"

using namespace pixel_engine::render_gl::systems;
using namespace pixel_engine::render_gl::components;

EPIX_API void ShaderPtr::create(int type) { id = glCreateShader(type); }

EPIX_API void ShaderPtr::source(const char* source) {
    glShaderSource(id, 1, &source, NULL);
}

EPIX_API bool ShaderPtr::compile() {
    glCompileShader(id);
    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    return success;
}

EPIX_API bool ShaderPtr::valid() const { return id != 0; }

EPIX_API void ProgramPtr::create() { id = glCreateProgram(); }

EPIX_API void ProgramPtr::attach(const ShaderPtr& shader) {
    glAttachShader(id, shader.id);
}

EPIX_API bool ProgramPtr::link() {
    glLinkProgram(id);
    int success;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    return success;
}

EPIX_API void ProgramPtr::use() const { glUseProgram(id); }

EPIX_API bool ProgramPtr::valid() const { return id != 0; }

EPIX_API VertexAttrib& VertexAttribs::operator[](size_t index) {
    return attribs[index];
}

EPIX_API void VertexAttribs::add(
    uint32_t location,
    uint32_t size,
    int type,
    bool normalized,
    uint32_t stride,
    uint64_t offset
) {
    attribs.push_back({location, size, type, normalized, stride, offset});
}

EPIX_API void VertexArrayPtr::create() { glCreateVertexArrays(1, &id); }

EPIX_API void VertexArrayPtr::bind() const { glBindVertexArray(id); }

EPIX_API bool VertexArrayPtr::valid() const { return id != 0; }

EPIX_API void BufferPtr::create() { glCreateBuffers(1, &id); }

EPIX_API void BufferPtr::bind(int target) const { glBindBuffer(target, id); }

EPIX_API void BufferPtr::bindBase(int target, int index) const {
    glBindBufferBase(target, index, id);
}

EPIX_API bool BufferPtr::valid() const { return id != 0; }

EPIX_API void BufferPtr::data(const void* data, size_t size, int usage) {
    glNamedBufferData(id, size, data, usage);
}

EPIX_API void BufferPtr::subData(const void* data, size_t size, size_t offset) {
    glNamedBufferSubData(id, offset, size, data);
}

EPIX_API void UniformBufferBindings::bind() const {
    for (size_t i = 0; i < buffers.size(); i++) {
        glBindBufferBase(GL_UNIFORM_BUFFER, i, buffers[i].id);
    }
}

EPIX_API void UniformBufferBindings::set(size_t index, const BufferPtr& buffer) {
    if (index >= buffers.size()) {
        buffers.resize(index + 1);
    }
    buffers[index] = buffer;
}

EPIX_API void StorageBufferBindings::bind() const {
    for (size_t i = 0; i < buffers.size(); i++) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, buffers[i].id);
    }
}

EPIX_API void StorageBufferBindings::set(size_t index, const BufferPtr& buffer) {
    if (index >= buffers.size()) {
        buffers.resize(index + 1);
    }
    buffers[index] = buffer;
}

EPIX_API BufferPtr& UniformBufferBindings::operator[](size_t index) {
    if (index >= buffers.size()) {
        buffers.resize(index + 1);
    }
    return buffers[index];
}

EPIX_API BufferPtr& StorageBufferBindings::operator[](size_t index) {
    if (index >= buffers.size()) {
        buffers.resize(index + 1);
    }
    return buffers[index];
}

EPIX_API void TexturePtr::create(int type) {
    this->type = type;
    glCreateTextures(type, 1, &id);
}

EPIX_API bool TexturePtr::valid() const { return id != 0; }

EPIX_API void SamplerPtr::create() { glCreateSamplers(1, &id); }

EPIX_API bool SamplerPtr::valid() const { return id != 0; }

EPIX_API void TextureBindings::bind() const {
    for (size_t i = 0; i < textures.size(); i++) {
        glBindTextureUnit(i, textures[i].texture.id);
        glBindSampler(i, textures[i].sampler.id);
    }
}

EPIX_API void TextureBindings::set(size_t index, const Image& texture) {
    if (index >= textures.size()) {
        textures.resize(index + 1);
    }
    textures[index] = texture;
}

EPIX_API Image& TextureBindings::operator[](size_t index) {
    if (index >= textures.size()) {
        textures.resize(index + 1);
    }
    return textures[index];
}

EPIX_API void ImageTextureBindings::bind() const {
    for (size_t i = 0; i < images.size(); i++) {
        glBindImageTexture(
            i, images[i].texture.id, images[i].level, images[i].layered,
            images[i].layer, images[i].access, images[i].format
        );
    }
}

EPIX_API void ImageTextureBindings::set(
    size_t index,
    const TexturePtr& texture,
    int level,
    bool layered,
    int layer,
    int access,
    int format
) {
    if (index >= images.size()) {
        images.resize(index + 1);
    }
    images[index] = {texture, level, layered, layer, access, format};
}

EPIX_API ImageTexture& ImageTextureBindings::operator[](size_t index) {
    if (index >= images.size()) {
        images.resize(index + 1);
    }
    return images[index];
}

EPIX_API void ViewPort::use() const { glViewport(x, y, width, height); }

EPIX_API void DepthRange::use() const { glDepthRange(nearf, farf); }

EPIX_API void PipelineLayout::use() const {
    vertex_array.bind();
    vertex_buffer.bind(GL_ARRAY_BUFFER);
    index_buffer.bind(GL_ELEMENT_ARRAY_BUFFER);
    draw_indirect_buffer.bind(GL_DRAW_INDIRECT_BUFFER);
    uniform_buffers.bind();
    textures.bind();
    storage_buffers.bind();
    images.bind();
}

EPIX_API void PerSampleOperations::use() const {
    if (scissor_test.enable) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(
            scissor_test.x, scissor_test.y, scissor_test.width,
            scissor_test.height
        );
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
    if (stencil_test.enable) {
        glEnable(GL_STENCIL_TEST);
        glStencilFuncSeparate(
            GL_FRONT, stencil_test.func_front.func, stencil_test.func_front.ref,
            stencil_test.func_front.mask
        );
        glStencilFuncSeparate(
            GL_BACK, stencil_test.func_back.func, stencil_test.func_back.ref,
            stencil_test.func_back.mask
        );
        glStencilOpSeparate(
            GL_FRONT, stencil_test.op_front.sfail, stencil_test.op_front.dpfail,
            stencil_test.op_front.dppass
        );
        glStencilOpSeparate(
            GL_BACK, stencil_test.op_back.sfail, stencil_test.op_back.dpfail,
            stencil_test.op_back.dppass
        );
    } else {
        glDisable(GL_STENCIL_TEST);
    }
    if (depth_test.enable) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(depth_test.func.func);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    if (blending.enable) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(
            blending.blend_func.src_rgb, blending.blend_func.dst_rgb,
            blending.blend_func.src_alpha, blending.blend_func.dst_alpha
        );
        glBlendEquationSeparate(
            blending.blend_equation.mode_rgb, blending.blend_equation.mode_alpha
        );
        glBlendColor(
            blending.blend_color.red, blending.blend_color.green,
            blending.blend_color.blue, blending.blend_color.alpha
        );
    } else {
        glDisable(GL_BLEND);
    }
    if (logic_op.enable) {
        glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(logic_op.op);
    } else {
        glDisable(GL_COLOR_LOGIC_OP);
    }
}

EPIX_API void RenderBufferPtr::create() { glCreateRenderbuffers(1, &id); }

EPIX_API bool RenderBufferPtr::valid() const { return id != 0; }

EPIX_API void RenderBufferPtr::storage(int internal_format, int width, int height) {
    glNamedRenderbufferStorage(id, internal_format, width, height);
}

EPIX_API bool RenderBufferPtr::check() {
    auto status = glCheckNamedFramebufferStatus(id, GL_RENDERBUFFER);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

EPIX_API void FrameBufferPtr::create() { glCreateFramebuffers(1, &id); }

EPIX_API void FrameBufferPtr::bind() const { glBindFramebuffer(GL_FRAMEBUFFER, id); }

EPIX_API bool FrameBufferPtr::check() {
    auto status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

EPIX_API bool FrameBufferPtr::unique() const { return id != 0; }

EPIX_API void FrameBufferPtr::attachTexture(
    uint32_t attachment, const TexturePtr& texture, int level
) {
    glNamedFramebufferTexture(id, attachment, texture.id, level);
}

EPIX_API void FrameBufferPtr::attachTextureLayer(
    uint32_t attachment, const TexturePtr& texture, int level, int layer
) {
    glNamedFramebufferTextureLayer(id, attachment, texture.id, level, layer);
}

EPIX_API void FrameBufferPtr::attachRenderBuffer(
    uint32_t attachment, const RenderBufferPtr& render_buffer
) {
    glNamedFramebufferRenderbuffer(
        id, attachment, GL_RENDERBUFFER, render_buffer.id
    );
}

EPIX_API void pixel_engine::render_gl::RenderGLPlugin::build(App& app) {
    using namespace render_gl;
    using namespace window;
    app.configure_sets(
           RenderGLStartupSets::context_creation,
           RenderGLStartupSets::after_context_creation
    )
        .configure_sets(
            RenderGLPipelineCompletionSets::pipeline_completion,
            RenderGLPipelineCompletionSets::after_pipeline_completion
        )
        .add_system(PreStartup, context_creation)
        .in_set(
            window::WindowStartUpSets::after_window_creation,
            RenderGLStartupSets::context_creation
        )
        .use_worker("single")
        ->add_system(PreRender, clear_color)
        .use_worker("single")
        ->add_system(PreRender, update_viewport)
        .use_worker("single")
        ->add_system(PreRender, complete_pipeline)
        .in_set(RenderGLPipelineCompletionSets::pipeline_completion)
        .use_worker("single")
        ->add_system(PostRender, swap_buffers)
        .use_worker("single");
}

EPIX_API void pixel_engine::render_gl::systems::clear_color(Query<Get<Window>> query) {
    for (auto [window] : query.iter()) {
        if (glfwGetCurrentContext() != window.get_handle())
            glfwMakeContextCurrent(window.get_handle());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

EPIX_API void pixel_engine::render_gl::systems::update_viewport(Query<Get<Window>> query
) {
    for (auto [window] : query.iter()) {
        if (glfwGetCurrentContext() != window.get_handle())
            glfwMakeContextCurrent(window.get_handle());
        glViewport(0, 0, window.get_size().width, window.get_size().height);
    }
}

EPIX_API void pixel_engine::render_gl::systems::context_creation(Query<Get<Window>> query
) {
    for (auto [window] : query.iter()) {
        if (glfwGetCurrentContext() != window.get_handle())
            glfwMakeContextCurrent(window.get_handle());
        gladLoadGL();
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(
            [](GLenum source, GLenum type, GLuint id, GLenum severity,
               GLsizei length, const GLchar* message, const void* userParam) {
                if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
                    spdlog::debug(
                        "OpenGL debug message: source: {}, type: {}, id: {}, "
                        "severity: {}, message: {}",
                        source, type, id, severity, message
                    );
                    return;
                }
                spdlog::warn(
                    "OpenGL debug message: source: {}, type: {}, id: {}, "
                    "severity: {}, message: {}",
                    source, type, id, severity, message
                );
            },
            nullptr
        );
    }
}

EPIX_API void pixel_engine::render_gl::systems::swap_buffers(Query<Get<Window>> query) {
    for (auto [window] : query.iter()) {
        if (glfwGetCurrentContext() != window.get_handle())
            glfwMakeContextCurrent(window.get_handle());
        glfwSwapBuffers(window.get_handle());
    }
}

EPIX_API void pixel_engine::render_gl::systems::complete_pipeline(
    Command command,
    Query<
        Get<Entity, ProgramShaderAttachments, VertexAttribs>,
        With<PipelineCreation>,
        Without<>> query
) {
    for (auto [id, shaders, attribs] : query.iter()) {
        spdlog::debug("Completing pipeline");

        BufferPtr vertex_buffer;
        vertex_buffer.create();
        vertex_buffer.bind(GL_ARRAY_BUFFER);

        VertexArrayPtr vertex_array;
        vertex_array.create();
        vertex_array.bind();

        PipelineLayout layout;

        layout.vertex_buffer = vertex_buffer;
        layout.vertex_array  = vertex_array;

        for (auto& attrib : attribs.attribs) {
            glEnableVertexAttribArray(attrib.location);
            glVertexAttribPointer(
                attrib.location, attrib.size, attrib.type, attrib.normalized,
                attrib.stride, (void*)attrib.offset
            );
        }

        ProgramPtr program;
        program.create();
        if (shaders.vertex_shader.valid()) {
            program.attach(shaders.vertex_shader);
        }
        if (shaders.fragment_shader.valid()) {
            program.attach(shaders.fragment_shader);
        }
        if (shaders.geometry_shader.valid()) {
            program.attach(shaders.geometry_shader);
        }
        if (shaders.tess_control_shader.valid()) {
            program.attach(shaders.tess_control_shader);
        }
        if (shaders.tess_evaluation_shader.valid()) {
            program.attach(shaders.tess_evaluation_shader);
        }
        auto success = program.link();

        auto entity_command = command.entity(id);

        entity_command.erase<PipelineCreation>();
        entity_command.erase<ProgramShaderAttachments>();
        entity_command.erase<VertexAttribs>();

        entity_command.emplace(PipelineBundle{
            .layout  = layout,
            .program = program,
        });
    }
}