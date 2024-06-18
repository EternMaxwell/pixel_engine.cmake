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
                Query<Get<Pipeline, Buffers, Images, T, ProgramLinked>, Without<>>,
                Query<Get<WindowHandle, WindowCreated, WindowSize, U>, Without<>>)>
                bind_pipeline = [](Query<Get<Pipeline, Buffers, Images, T, ProgramLinked>, Without<>> query,
                                   Query<Get<WindowHandle, WindowCreated, WindowSize, U>, Without<>> window_query) {
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

            void clear_color(Query<Get<WindowHandle, WindowCreated>, Without<>> query) {
                for (auto [window_handle] : query.iter()) {
                    glfwMakeContextCurrent(window_handle.window_handle);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                }
            }

            void update_viewport(Query<Get<WindowHandle, WindowCreated, WindowSize>, Without<>> query) {
                for (auto [window_handle, window_size] : query.iter()) {
                    glfwMakeContextCurrent(window_handle.window_handle);
                    glViewport(0, 0, window_size.width, window_size.height);
                }
            }

            void context_creation(Query<Get<WindowHandle, WindowCreated>, Without<>> query) {
                for (auto [window_handle] : query.iter()) {
                    glfwMakeContextCurrent(window_handle.window_handle);
                    gladLoadGL();
                    glEnable(GL_DEPTH_TEST);
                    glEnable(GL_BLEND);
                }
            }

            void create_pipelines(
                Command command, Query<Get<entt::entity, Pipeline, Shaders, Attribs>, Without<ProgramLinked>> query) {
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
                            attrib.location, attrib.size, attrib.type, attrib.normalized, attrib.stride,
                            (void*)attrib.offset);
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
        }  // namespace systems
    }      // namespace render_gl
}  // namespace pixel_engine