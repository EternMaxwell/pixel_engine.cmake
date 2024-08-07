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

pipeline::VertexAttrib& pipeline::VertexAttribs::operator[](size_t index) {
    return attribs[index];
}

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

void pipeline::BufferPtr::data(const void* data, size_t size, int usage) {
    glNamedBufferData(id, size, data, usage);
}

void pipeline::BufferPtr::subData(
    const void* data, size_t size, size_t offset) {
    glNamedBufferSubData(id, offset, size, data);
}

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

void PerSampleOperations::use() const {
    if (scissor_test.enable) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(
            scissor_test.x, scissor_test.y, scissor_test.width,
            scissor_test.height);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
    if (stencil_test.enable) {
        glEnable(GL_STENCIL_TEST);
        glStencilFuncSeparate(
            GL_FRONT, stencil_test.func_front.func, stencil_test.func_front.ref,
            stencil_test.func_front.mask);
        glStencilFuncSeparate(
            GL_BACK, stencil_test.func_back.func, stencil_test.func_back.ref,
            stencil_test.func_back.mask);
        glStencilOpSeparate(
            GL_FRONT, stencil_test.op_front.sfail, stencil_test.op_front.dpfail,
            stencil_test.op_front.dppass);
        glStencilOpSeparate(
            GL_BACK, stencil_test.op_back.sfail, stencil_test.op_back.dpfail,
            stencil_test.op_back.dppass);
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
            blending.blend_func.src_alpha, blending.blend_func.dst_alpha);
        glBlendEquationSeparate(
            blending.blend_equation.mode_rgb,
            blending.blend_equation.mode_alpha);
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

void RenderBufferPtr::create() { glCreateRenderbuffers(1, &id); }

bool RenderBufferPtr::valid() const { return id != 0; }

void RenderBufferPtr::storage(int internal_format, int width, int height) {
    glNamedRenderbufferStorage(id, internal_format, width, height);
}

bool RenderBufferPtr::check() {
    auto status = glCheckNamedFramebufferStatus(id, GL_RENDERBUFFER);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

void FrameBufferPtr::create() { glCreateFramebuffers(1, &id); }

void FrameBufferPtr::bind() const { glBindFramebuffer(GL_FRAMEBUFFER, id); }

bool FrameBufferPtr::check() {
    auto status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

bool FrameBufferPtr::unique() const { return id != 0; }

void FrameBufferPtr::attachTexture(
    uint32_t attachment, const TexturePtr& texture, int level) {
    glNamedFramebufferTexture(id, attachment, texture.id, level);
}

void FrameBufferPtr::attachTextureLayer(
    uint32_t attachment, const TexturePtr& texture, int level, int layer) {
    glNamedFramebufferTextureLayer(id, attachment, texture.id, level, layer);
}

void FrameBufferPtr::attachRenderBuffer(
    uint32_t attachment, const RenderBufferPtr& render_buffer) {
    glNamedFramebufferRenderbuffer(
        id, attachment, GL_RENDERBUFFER, render_buffer.id);
}

void DrawArraysData::write(const void* rdata, size_t size, size_t offset) {
    if (offset + size > data.size()) {
        data.resize(offset + size);
    }
    memcpy(data.data() + offset, rdata, size);
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
        .configure_sets(
            RenderGLPipelineCompletionSets::pipeline_completion,
            RenderGLPipelineCompletionSets::after_pipeline_completion)
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
        .add_system_main(PreRender{}, update_viewport)
        .add_system_main(
            PostStartup{}, complete_pipeline,
            in_set(RenderGLPipelineCompletionSets::pipeline_completion))
        .add_system_main(
            PreRender{}, complete_pipeline,
            in_set(RenderGLPreRenderSets::create_pipelines))
        .add_system_main(PostRender{}, draw_arrays)
        .add_system_main(PreRender{}, update_viewports);
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
        glad_set_post_callback(
            [](const char* name, void* funcptr, int len_args, ...) {
                auto error = glad_glGetError();
                if (error != GL_NO_ERROR) {
                    spdlog::warn(
                        "OpenGL error: {} at {}", error, name);
                }
            });
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
    }
}

void pixel_engine::render_gl::systems::create_pipelines(
    Command command,
    Query<
        Get<entt::entity, components::Pipeline, Shaders, Attribs>, With<>,
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
                         With<pipeline::PipelineCreation>, Without<>>
                         query) {
    for (auto [id, shaders, attribs] : query.iter()) {
        spdlog::debug("Completing pipeline");

        BufferPtr vertex_buffer;
        vertex_buffer.create();
        vertex_buffer.bind(GL_ARRAY_BUFFER);

        VertexArrayPtr vertex_array;
        vertex_array.create();
        vertex_array.bind();

        BufferBindings buffer_bindings;

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

        auto entity_command = command.entity(id);

        entity_command.emplace(pipeline::PipelineBundle{
            .vertex_array = vertex_array,
            .buffers = buffer_bindings,
            .program = program,
        });

        entity_command.erase<pipeline::PipelineCreation>();
        entity_command.erase<pipeline::ProgramShaderAttachments>();
        entity_command.erase<pipeline::VertexAttribs>();
    }
}

void pixel_engine::render_gl::systems::use_pipeline(
    const pipeline::VertexArrayPtr& vertex_array,
    const pipeline::BufferBindings& buffers,
    const pipeline::ProgramPtr& program) {
    vertex_array.bind();
    program.use();
    buffers.bind();
}

void pixel_engine::render_gl::systems::use_pipeline(
    entt::entity pipeline_entity,
    Query<
        Get<const pipeline::VertexArrayPtr, pipeline::BufferBindings,
            const pipeline::ProgramPtr>,
        With<pipeline::Pipeline>, Without<>>& query) {
    auto pipeline = query.get(pipeline_entity);
    auto& [vertex_array, buffers, program] = pipeline;
    pixel_engine::render_gl::systems::use_pipeline(
        vertex_array, buffers, program);
}

void pixel_engine::render_gl::systems::use_layout(
    entt::entity layout_entity,
    Query<
        Get<const pipeline::UniformBufferBindings,
            const pipeline::StorageBufferBindings,
            const pipeline::TextureBindings,
            const pipeline::ImageTextureBindings>,
        With<pipeline::PipelineLayout>, Without<>>& query) {
    auto
        [uniform_buffer_bindings, storage_buffer_bindings, texture_bindings,
         image_texture_bindings] = query.get(layout_entity);
    uniform_buffer_bindings.bind();
    storage_buffer_bindings.bind();
    texture_bindings.bind();
    image_texture_bindings.bind();
}

void pixel_engine::render_gl::systems::draw_arrays(
    Query<
        Get<entt::entity, const pipeline::RenderPassPtr,
            const pipeline::DrawArrays>>
        draw_query,
    Query<
        Get<const pipeline::PipelinePtr, const pipeline::FrameBufferPtr,
            const pipeline::ViewPort, const pipeline::DepthRange,
            const pipeline::PipelineLayoutPtr,
            const pipeline::PerSampleOperations>,
        With<pipeline::RenderPass>, Without<>>
        render_pass_query,
    Query<
        Get<const pipeline::VertexArrayPtr, pipeline::BufferBindings,
            const pipeline::ProgramPtr>,
        With<pipeline::Pipeline>, Without<>>
        pipeline_query,
    Query<
        Get<const pipeline::UniformBufferBindings,
            const pipeline::StorageBufferBindings,
            const pipeline::TextureBindings,
            const pipeline::ImageTextureBindings>,
        With<pipeline::PipelineLayout>, Without<>>
        layout_query,
    Query<Get<const pipeline::DrawArraysData>> draw_arrays_data_query) {
    for (auto [entity, render_pass, draw_arrays] : draw_query.iter()) {
        auto
            [pipeline, frame_buffer, view_port, depth_range, layout,
             per_sample_operations] =
                render_pass_query.get(render_pass.render_pass);
        use_pipeline(pipeline.pipeline, pipeline_query);
        use_layout(layout.layout, layout_query);
        frame_buffer.bind();
        glViewport(view_port.x, view_port.y, view_port.width, view_port.height);
        glDepthRange(depth_range.nearf, depth_range.farf);
        per_sample_operations.use();

        if (draw_arrays_data_query.contains(entity)) {
            auto [draw_arrays_data] = draw_arrays_data_query.get(entity);
            auto [vertex_array, buffers, program] =
                pipeline_query.get(pipeline.pipeline);
            glNamedBufferData(
                buffers.vertex_buffer.id, draw_arrays_data.data.size(),
                draw_arrays_data.data.data(), GL_STATIC_DRAW);
        }
        glDrawArrays(draw_arrays.mode, draw_arrays.first, draw_arrays.count);
    }
}

void pixel_engine::render_gl::systems::update_viewports(
    Query<Get<WindowSize>, With<WindowCreated>, Without<>> window_query,
    Query<Get<pipeline::ViewPort>, With<pipeline::RenderPass>, Without<>>
        viewport_query) {
    for (auto [window_size] : window_query.iter()) {
        for (auto [view_port] : viewport_query.iter()) {
            view_port.width = window_size.width;
            view_port.height = window_size.height;
            view_port.x = 0;
            view_port.y = 0;
        }
    }
}