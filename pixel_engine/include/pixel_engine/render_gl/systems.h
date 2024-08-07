#pragma once

#include <glad/glad.h>

#include "components.h"
#include "pixel_engine/entity.h"
#include "pixel_engine/window/window.h"

namespace pixel_engine {
namespace render_gl {
namespace systems {
using namespace prelude;
using namespace components;
using namespace window::components;

template <typename T, typename U>
std::function<void(
    Query<Get<Pipeline, Buffers, Images>, With<T, ProgramLinked>, Without<>>,
    Query<Get<WindowHandle, WindowSize>, With<WindowCreated, U>, Without<>>)>
    bind_pipeline =
        [](Query<
               Get<Pipeline, Buffers, Images>, With<T, ProgramLinked>,
               Without<>>
               query,
           Query<
               Get<WindowHandle, WindowSize>, With<WindowCreated, U>, Without<>>
               window_query) {
            for (auto [window_handle, window_size] : window_query.iter()) {
                for (auto [pipeline, buffers, images] : query.iter()) {
                    glUseProgram(pipeline.program);
                    glBindBuffer(
                        GL_ARRAY_BUFFER, pipeline.vertex_buffer.buffer);
                    glBindVertexArray(pipeline.vertex_array);
                    glNamedBufferData(
                        pipeline.vertex_buffer.buffer,
                        pipeline.vertex_buffer.data.size(),
                        pipeline.vertex_buffer.data.data(), GL_DYNAMIC_DRAW);
                    int index = 0;
                    for (auto& image : images.images) {
                        glBindTextureUnit(index, image.texture);
                        glBindSampler(index, image.sampler);
                        index++;
                    }
                    index = 0;
                    for (auto& buffer : buffers.uniform_buffers) {
                        glBindBufferBase(
                            GL_UNIFORM_BUFFER, index, buffer.buffer);
                        glNamedBufferData(
                            buffer.buffer, buffer.data.size(),
                            buffer.data.data(), GL_DYNAMIC_DRAW);
                        index++;
                    }
                    index = 0;
                    for (auto& buffer : buffers.storage_buffers) {
                        glBindBufferBase(
                            GL_SHADER_STORAGE_BUFFER, index, buffer.buffer);
                        glNamedBufferData(
                            buffer.buffer, buffer.data.size(),
                            buffer.data.data(), GL_DYNAMIC_DRAW);
                        index++;
                    }
                    glViewport(0, 0, window_size.width, window_size.height);
                    glDrawArrays(GL_TRIANGLES, 0, pipeline.vertex_count);
                    return;
                }
            }
        };

void clear_color(
    Query<Get<WindowHandle>, With<WindowCreated>, Without<>> query);

void update_viewport(
    Query<Get<WindowHandle, WindowSize>, With<WindowCreated>, Without<>> query);

void context_creation(
    Query<Get<WindowHandle>, With<WindowCreated>, Without<>> query);

void create_pipelines(
    Command command, Query<
                         Get<entt::entity, Pipeline, Shaders, Attribs>, With<>,
                         Without<ProgramLinked>>
                         query);

void complete_pipeline(
    Command command, Query<
                         Get<entt::entity, pipeline::ProgramShaderAttachments,
                             pipeline::VertexAttribs>,
                         With<pipeline::PipelineCreation>, Without<>>
                         query);

void use_pipeline(
    const pipeline::VertexArrayPtr& vertex_array,
    const pipeline::BufferBindings& buffers,
    const pipeline::ProgramPtr& program);

void use_pipeline(
    entt::entity pipeline_entity,
    Query<
        Get<const pipeline::VertexArrayPtr, pipeline::BufferBindings,
            const pipeline::ProgramPtr>,
        With<pipeline::Pipeline>, Without<>>& query);

void use_layout(
    entt::entity layout_entity,
    Query<
        Get<const pipeline::UniformBufferBindings,
            const pipeline::StorageBufferBindings,
            const pipeline::TextureBindings,
            const pipeline::ImageTextureBindings>,
        With<pipeline::PipelineLayout>, Without<>>& query);

void draw_arrays(
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
    Query<Get<const pipeline::DrawArraysData>> draw_arrays_data_query);

void update_viewports(
    Query<Get<WindowSize>, With<WindowCreated>, Without<>>
        window_query,
    Query<Get<pipeline::ViewPort>, With<pipeline::RenderPass>, Without<>>
        viewport_query);
}  // namespace systems
}  // namespace render_gl
}  // namespace pixel_engine