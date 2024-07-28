#include "pixel_engine/render_gl/components.h"
#include "pixel_engine/render_gl/render_gl.h"
#include "pixel_engine/render_gl/systems.h"

using namespace pixel_engine::render_gl::systems;
using namespace pixel_engine::render_gl::components;
using namespace pipeline;

void ShaderPtr::create(int type) { id = glCreateShader(type); }

void ShaderPtr::source(const char* source) {
    glShaderSource(id, 1, &source, NULL);
}

bool ShaderPtr::compile() {
    glCompileShader(id);
    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    return success;
}

bool ShaderPtr::valid() const { return id != 0; }

void ProgramPtr::create() { id = glCreateProgram(); }

void ProgramPtr::attach(const ShaderPtr& shader) {
    glAttachShader(id, shader.id);
}

bool ProgramPtr::link() {
    glLinkProgram(id);
    int success;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    return success;
}

void ProgramPtr::use() const { glUseProgram(id); }

bool ProgramPtr::valid() const { return id != 0; }

void VertexAttribs::add(
    uint32_t location, uint32_t size, int type, bool normalized,
    uint32_t stride, uint64_t offset) {
    attribs.push_back({location, size, type, normalized, stride, offset});
}

void VertexArrayPtr::create() { glCreateVertexArrays(1, &id); }

void VertexArrayPtr::bind() const { glBindVertexArray(id); }

bool VertexArrayPtr::valid() const { return id != 0; }

void pipeline::BufferPtr::create() { glCreateBuffers(1, &id); }

void pipeline::BufferPtr::bind(int target) const { glBindBuffer(target, id); }

void pipeline::BufferPtr::bindBase(int target, int index) {
    glBindBufferBase(target, index, id);
}

bool pipeline::BufferPtr::valid() const { return id != 0; }

void UniformBufferBindings::bind() const {
    for (size_t i = 0; i < buffers.size(); i++) {
        glBindBufferBase(GL_UNIFORM_BUFFER, i, buffers[i].id);
    }
}

void UniformBufferBindings::set(size_t index, const BufferPtr& buffer) {
    if (index >= buffers.size()) {
        buffers.resize(index + 1);
    }
    buffers[index] = buffer;
}

void StorageBufferBindings::bind() const {
    for (size_t i = 0; i < buffers.size(); i++) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, buffers[i].id);
    }
}

void StorageBufferBindings::set(size_t index, const BufferPtr& buffer) {
    if (index >= buffers.size()) {
        buffers.resize(index + 1);
    }
    buffers[index] = buffer;
}

pipeline::BufferPtr& UniformBufferBindings::operator[](size_t index) {
    return buffers[index];
}

pipeline::BufferPtr& StorageBufferBindings::operator[](size_t index) {
    return buffers[index];
}

void TexturePtr::create(int type) {
    this->type = type;
    glCreateTextures(type, 1, &id);
}

bool TexturePtr::valid() const { return id != 0; }

void SamplerPtr::create() { glCreateSamplers(1, &id); }

bool SamplerPtr::valid() const { return id != 0; }

void TextureBindings::bind() const {
    for (size_t i = 0; i < textures.size(); i++) {
        glBindTextureUnit(i, textures[i].texture.id);
        glBindSampler(i, textures[i].sampler.id);
    }
}

void TextureBindings::set(size_t index, const Image& texture) {
    if (index >= textures.size()) {
        textures.resize(index + 1);
    }
    textures[index] = texture;
}

pipeline::Image& TextureBindings::operator[](size_t index) {
    return textures[index];
}

void ImageTextureBindings::bind() const {
    for (size_t i = 0; i < images.size(); i++) {
        glBindImageTexture(
            i, images[i].texture.id, images[i].level, images[i].layered,
            images[i].layer, images[i].access, images[i].format);
    }
}

void ImageTextureBindings::set(
    size_t index, const TexturePtr& texture, int level, bool layered, int layer,
    int access, int format) {
    if (index >= images.size()) {
        images.resize(index + 1);
    }
    images[index] = {texture, level, layered, layer, access, format};
}

pipeline::ImageTexture& ImageTextureBindings::operator[](size_t index) {
    return images[index];
}

void BufferBindings::bind() const {
    if (vertex_buffer.valid()) {
        vertex_buffer.bind(GL_ARRAY_BUFFER);
    }
    if (index_buffer.valid()) {
        index_buffer.bind(GL_ELEMENT_ARRAY_BUFFER);
    }
    if (draw_indirect_buffer.valid()) {
        draw_indirect_buffer.bind(GL_DRAW_INDIRECT_BUFFER);
    }
}

void pixel_engine::render_gl::components::Buffer::write(
    const void* rdata, size_t size, size_t offset) {
    if (offset + size > data.size()) {
        data.resize(offset + size);
    }
    memcpy(data.data() + offset, rdata, size);
}

void pixel_engine::render_gl::RenderGLPlugin::build(App& app) {
    using namespace render_gl;
    using namespace window;
    app.configure_sets(
           RenderGLStartupSets::context_creation,
           RenderGLStartupSets::after_context_creation)
        .add_system_main(
            PreStartup{}, context_creation,
            in_set(
                window::WindowStartUpSets::after_window_creation,
                RenderGLStartupSets::context_creation))
        .configure_sets(
            RenderGLPreRenderSets::create_pipelines,
            RenderGLPreRenderSets::after_create_pipelines)
        .add_system_main(
            PreRender{}, create_pipelines,
            in_set(RenderGLPreRenderSets::create_pipelines))
        .add_system_main(PreRender{}, clear_color)
        .add_system_main(PreRender{}, update_viewport);
}

void pixel_engine::render_gl::systems::clear_color(
    Query<Get<WindowHandle>, With<WindowCreated>, Without<>> query) {
    for (auto [window_handle] : query.iter()) {
        if (glfwGetCurrentContext() != window_handle.window_handle)
            glfwMakeContextCurrent(window_handle.window_handle);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void pixel_engine::render_gl::systems::update_viewport(
    Query<Get<WindowHandle, WindowSize>, With<WindowCreated>, Without<>>
        query) {
    for (auto [window_handle, window_size] : query.iter()) {
        if (glfwGetCurrentContext() != window_handle.window_handle)
            glfwMakeContextCurrent(window_handle.window_handle);
        glViewport(0, 0, window_size.width, window_size.height);
    }
}

void pixel_engine::render_gl::systems::context_creation(
    Query<Get<WindowHandle>, With<WindowCreated>, Without<>> query) {
    for (auto [window_handle] : query.iter()) {
        if (glfwGetCurrentContext() != window_handle.window_handle)
            glfwMakeContextCurrent(window_handle.window_handle);
        gladLoadGL();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
    }
}

void pixel_engine::render_gl::systems::create_pipelines(
    Command command, Query<
                         Get<entt::entity, Pipeline, Shaders, Attribs>, With<>,
                         Without<ProgramLinked>>
                         query) {
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
                attrib.location, attrib.size, attrib.type, attrib.normalized,
                attrib.stride, (void*)attrib.offset);
        }
        glLinkProgram(pipeline.program);
        int success;
        glGetProgramiv(pipeline.program, GL_LINK_STATUS, &success);
        if (!success) {
            char info_log[512];
            glGetProgramInfoLog(pipeline.program, 512, NULL, info_log);
            throw std::runtime_error(
                "Failed to link program: " + std::string(info_log));
        }
        command.entity(id).emplace(ProgramLinked{});
        spdlog::debug("End creating new pipeline.");
    }
}

void pixel_engine::render_gl::systems::complete_pipeline(
    Command command, Query<
                         Get<entt::entity, pipeline::ProgramShaderAttachments,
                             pipeline::VertexAttribs>,
                         With<>, Without<>>
                         query) {
    for (auto [id, shaders, attribs] : query.iter()) {
        spdlog::debug("Completing pipeline");

        VertexArrayPtr vertex_array;
        vertex_array.create();
        vertex_array.bind();

        BufferBindings buffer_bindings;

        BufferPtr vertex_buffer;
        vertex_buffer.create();
        vertex_buffer.bind(GL_ARRAY_BUFFER);

        buffer_bindings.vertex_buffer = vertex_buffer;

        for (auto& attrib : attribs.attribs) {
            glEnableVertexAttribArray(attrib.location);
            glVertexAttribPointer(
                attrib.location, attrib.size, attrib.type, attrib.normalized,
                attrib.stride, (void*)attrib.offset);
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
        program.link();

        command.spawn(pipeline::PipelineBundle{
            .vertex_array = vertex_array,
            .buffers = buffer_bindings,
            .program = program,
        });

        command.entity(id).despawn();
    }
}

void pixel_engine::render_gl::systems::use_pipeline(
    const pipeline::VertexArrayPtr& vertex_array,
    const pipeline::BufferBindings& buffers,
    const pipeline::UniformBufferBindings& uniform_buffers,
    const pipeline::StorageBufferBindings& storage_buffers,
    const pipeline::TextureBindings& textures,
    const pipeline::ImageTextureBindings& images,
    const pipeline::ProgramPtr& program, const pipeline::ViewPort& viewport,
    const pipeline::DepthRange& depth_range,
    const pipeline::ScissorTest& scissor_test,
    const pipeline::DepthTest& depth_test,
    const pipeline::StencilTest& stencil_test,
    const pipeline::Blending& blending, const pipeline::LogicOp& logic_op) {
    vertex_array.bind();
    program.use();
    buffers.bind();
    uniform_buffers.bind();
    storage_buffers.bind();
    textures.bind();
    images.bind();
    glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    glDepthRange(depth_range.nearf, depth_range.farf);
    if (scissor_test.enable) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(
            scissor_test.x, scissor_test.y, scissor_test.width,
            scissor_test.height);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
    if (depth_test.enable) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(depth_test.func.func);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    if (stencil_test.enable) {
        glEnable(GL_STENCIL_TEST);
        glStencilFuncSeparate(
            GL_FRONT, stencil_test.func_front.func, stencil_test.func_front.ref,
            stencil_test.func_front.mask);
        glStencilOpSeparate(
            GL_FRONT, stencil_test.op_front.sfail, stencil_test.op_front.dpfail,
            stencil_test.op_front.dppass);
        glStencilFuncSeparate(
            GL_BACK, stencil_test.func_back.func, stencil_test.func_back.ref,
            stencil_test.func_back.mask);
        glStencilOpSeparate(
            GL_BACK, stencil_test.op_back.sfail, stencil_test.op_back.dpfail,
            stencil_test.op_back.dppass);
    } else {
        glDisable(GL_STENCIL_TEST);
    }
    if (blending.enable) {
        glEnable(GL_BLEND);
        glBlendEquationSeparate(
            blending.blend_equation.mode_rgb,
            blending.blend_equation.mode_alpha);
        glBlendFuncSeparate(
            blending.blend_func.src_rgb, blending.blend_func.dst_rgb,
            blending.blend_func.src_alpha, blending.blend_func.dst_alpha);
        glBlendColor(
            blending.blend_color.red, blending.blend_color.green,
            blending.blend_color.blue, blending.blend_color.alpha);
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

void pixel_engine::render_gl::systems::use_pipeline(
    entt::entity pipeline_entity,
    Query<
        Get<const pipeline::VertexArrayPtr, const pipeline::BufferBindings,
            const pipeline::UniformBufferBindings,
            const pipeline::StorageBufferBindings,
            const pipeline::TextureBindings,
            const pipeline::ImageTextureBindings, const pipeline::ProgramPtr,
            const pipeline::ViewPort, pipeline::DepthRange,
            const pipeline::ScissorTest, const pipeline::DepthTest,
            pipeline::StencilTest, const pipeline::Blending,
            const pipeline::LogicOp>,
        With<>, Without<>>& query) {
    auto pipeline = query.get(pipeline_entity);
    auto& [vertex_array, buffers, uniform_buffers, storage_buffers, textures, images, program, viewport, depth_range, scissor_test, depth_test, stencil_test, blending, logic_op] =
        pipeline;
    pixel_engine::render_gl::systems::use_pipeline(
        vertex_array, buffers, uniform_buffers, storage_buffers, textures,
        images, program, viewport, depth_range, scissor_test, depth_test,
        stencil_test, blending, logic_op);
}
