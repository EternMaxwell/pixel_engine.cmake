#pragma once

#ifdef NDEBUG
#include <glad/gl.h>
#else
#include <glad/glad.h>
#endif
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "pixel_engine/entity.h"
#include "pixel_engine/plugins/window_gl.h"

namespace pixel_engine {
    namespace plugins {
        namespace render_gl {
            struct Buffer {
                unsigned int buffer = 0;
                std::vector<uint8_t> data;
                void write(const void* data, size_t size, size_t offset) {
                    if (offset + size > this->data.size()) {
                        this->data.resize(offset + size);
                    }
                    memcpy(this->data.data() + offset, data, size);
                }
            };

            struct Pipeline {
                int program = 0;
                unsigned int vertex_array = 0;
                int vertex_count = 0;
                Buffer vertex_buffer = {};
                int width = 1920;
                int height = 1080;
            };

            struct Shaders {
                int vertex_shader = 0;
                int fragment_shader = 0;
                int geometry_shader = 0;
                int tess_control_shader = 0;
                int tess_evaluation_shader = 0;
            };

            struct Buffers {
                std::vector<Buffer> uniform_buffers;
                std::vector<Buffer> storage_buffers;
            };

            struct Image {
                unsigned int texture = 0;
                unsigned int sampler = 0;
            };

            struct Images {
                std::vector<Image> images;
            };

            struct VertexAttrib {
                int location;
                int size;
                int type;
                bool normalized;
                int stride;
                int offset;
            };

            struct Attribs {
                std::vector<VertexAttrib> attribs;
            };

            struct PipelineBundle {
                Bundle bundle;
                Pipeline pipeline;
                Shaders shaders;
                Attribs attribs;
                Buffers buffers;
                Images images;
            };

            struct ProgramLinked {};

            using namespace plugins::window_gl;

            template <typename T, typename U>
            std::function<void(Query<std::tuple<Pipeline, Buffers, Images, T, ProgramLinked>, std::tuple<>>,
                               Query<std::tuple<WindowHandle, WindowCreated, WindowSize, U>, std::tuple<>>)>
                bind_pipeline =
                    [](Query<std::tuple<Pipeline, Buffers, Images, T, ProgramLinked>, std::tuple<>> query,
                       Query<std::tuple<WindowHandle, WindowCreated, WindowSize, U>, std::tuple<>> window_query) {
                        for (auto [window_handle, window_size] : window_query.iter()) {
                            for (auto [pipeline, buffers, images] : query.iter()) {
                                glUseProgram(pipeline.program);
                                glBindBuffer(GL_ARRAY_BUFFER, pipeline.vertex_buffer.buffer);
                                glBindVertexArray(pipeline.vertex_array);
                                glNamedBufferData(pipeline.vertex_buffer.buffer, pipeline.vertex_buffer.data.size(),
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
                                    glNamedBufferData(buffer.buffer, buffer.data.size(), buffer.data.data(),
                                                      GL_DYNAMIC_DRAW);
                                    index++;
                                }
                                index = 0;
                                for (auto& buffer : buffers.storage_buffers) {
                                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.buffer);
                                    glNamedBufferData(buffer.buffer, buffer.data.size(), buffer.data.data(),
                                                      GL_DYNAMIC_DRAW);
                                    index++;
                                }
                                glViewport(0, 0, window_size.width, window_size.height);
                                glDrawArrays(GL_TRIANGLES, 0, pipeline.vertex_count);
                                return;
                            }
                        }
                    };

            void create_pipelines(
                Command command,
                Query<std::tuple<entt::entity, Pipeline, Shaders, Attribs>, std::tuple<ProgramLinked>> query) {
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
                        glVertexAttribPointer(attrib.location, attrib.size, attrib.type, attrib.normalized,
                                              attrib.stride, (void*)attrib.offset);
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
        }  // namespace render_gl

        class RenderGLPlugin : public entity::Plugin {
           public:
            Node create_pipelines_node;

            void build(App& app) override {
                using namespace render_gl;
                app.add_system_main(PreRender{}, create_pipelines, &create_pipelines_node);
            }
        };
    }  // namespace plugins
}  // namespace pixel_engine