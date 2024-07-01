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
                    [](Query<Get<Pipeline, Buffers, Images>, With<T, ProgramLinked>, Without<>> query,
                       Query<Get<WindowHandle, WindowSize>, With<WindowCreated, U>, Without<>> window_query) {
                        for (auto [window_handle, window_size] : window_query.iter()) {
                            for (auto [pipeline, buffers, images] : query.iter()) {
                                glUseProgram(pipeline.program);
                                glBindBuffer(GL_ARRAY_BUFFER, pipeline.vertex_buffer.buffer);
                                glBindVertexArray(pipeline.vertex_array);
                                glNamedBufferData(
                                    pipeline.vertex_buffer.buffer, pipeline.vertex_buffer.data.size(),
                                    pipeline.vertex_buffer.data.data(), GL_DYNAMIC_DRAW);
                                int index = 0;
                                for (auto& image : images.images) {
                                    glBindTextureUnit(index, image.texture);
                                    glBindSampler(index, image.sampler);
                                    index++;
                                }
                                index = 0;
                                for (auto& buffer : buffers.uniform_buffers) {
                                    glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.buffer);
                                    glNamedBufferData(
                                        buffer.buffer, buffer.data.size(), buffer.data.data(), GL_DYNAMIC_DRAW);
                                    index++;
                                }
                                index = 0;
                                for (auto& buffer : buffers.storage_buffers) {
                                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.buffer);
                                    glNamedBufferData(
                                        buffer.buffer, buffer.data.size(), buffer.data.data(), GL_DYNAMIC_DRAW);
                                    index++;
                                }
                                glViewport(0, 0, window_size.width, window_size.height);
                                glDrawArrays(GL_TRIANGLES, 0, pipeline.vertex_count);
                                return;
                            }
                        }
                    };

            void clear_color(Query<Get<WindowHandle>, With<WindowCreated>, Without<>> query);

            void update_viewport(Query<Get<WindowHandle, WindowSize>, With<WindowCreated>, Without<>> query);

            void context_creation(Query<Get<WindowHandle>, With<WindowCreated>, Without<>> query);

            void create_pipelines(
                Command command,
                Query<Get<entt::entity, Pipeline, Shaders, Attribs>, With<>, Without<ProgramLinked>> query);
        }  // namespace systems
    }      // namespace render_gl
}  // namespace pixel_engine