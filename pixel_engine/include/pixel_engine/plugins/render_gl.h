#pragma once

#include <glad/glad.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "pixel_engine/entity.h"
#include "pixel_engine/plugins/window_gl.h"

namespace pixel_engine {
    namespace plugins {
        namespace render_gl {

            /*! @brief A buffer object.
             * This struct represents a buffer object in OpenGL.
             * @brief buffer is the OpenGL buffer id.
             * @brief data is the data stored in the buffer. It is resized when the data written to it is larger than
             * the current size.
             */
            struct Buffer {
                /*! @brief The OpenGL buffer id. */
                unsigned int buffer = 0;
                /*! @brief The data stored in the buffer. */
                std::vector<uint8_t> data;
                /*! @brief Writes data to the buffer.
                 * @param data The data to write to the buffer.
                 * @param size The size of the data to write.
                 * @param offset The offset in the buffer to write the data to.
                 */
                void write(const void* data, size_t size, size_t offset) {
                    if (offset + size > this->data.size()) {
                        this->data.resize(offset + size);
                    }
                    memcpy(this->data.data() + offset, data, size);
                }
            };

            /*! @brief A pipeline object.
             * This struct represents a pipeline object in OpenGL.
             * @brief program is the OpenGL program id.
             * @brief vertex_array is the OpenGL vertex array id.
             * @brief vertex_count is the number of vertices to draw.
             * @brief vertex_buffer is the buffer object containing the vertex data.
             */
            struct Pipeline {
                /*! @brief The OpenGL program id. Not need to be initialized by user. */
                int program = 0;
                /*! @brief The OpenGL vertex array id. Not need to be initialized by user. */
                unsigned int vertex_array = 0;
                /*! @brief The number of vertices to draw. Not need to be initialized by user. */
                int vertex_count = 0;
                /*! @brief The buffer object containing the vertex data. Not need to be initialized by user. */
                Buffer vertex_buffer = {};
            };

            /*! @brief shaders used in a pipeline.
             * @brief vertex_shader is the OpenGL vertex shader id.
             * @brief fragment_shader is the OpenGL fragment shader id.
             * @brief geometry_shader is the OpenGL geometry shader id.
             * @brief tess_control_shader is the OpenGL tessellation control shader id.
             * @brief tess_evaluation_shader is the OpenGL tessellation evaluation shader id.
             * @brief 0 if no shader assigned. Assign shader by asset server is recommended.
             */
            struct Shaders {
                int vertex_shader = 0;
                int fragment_shader = 0;
                int geometry_shader = 0;
                int tess_control_shader = 0;
                int tess_evaluation_shader = 0;
            };

            /*! @brief Buffers used in a pipeline.
             * @brief uniform_buffers is a vector of buffer objects used as uniform buffers.
             * @brief storage_buffers is a vector of buffer objects used as storage buffers.
             * @brief index of the buffer object in the vector is the binding point in the shader.
             */
            struct Buffers {
                std::vector<Buffer> uniform_buffers;
                std::vector<Buffer> storage_buffers;
            };

            /*! @brief An image object.
             * This struct represents an image object in OpenGL.
             * @brief texture is the OpenGL texture id.
             * @brief sampler is the OpenGL sampler id.
             * @brief 0 if no image assigned. Assign image by asset server is recommended.
             */
            struct Image {
                int texture = 0;
                int sampler = 0;
            };

            /*! @brief Images used in a pipeline.
             * @brief images is a vector of image objects.
             * @brief index of the image object in the vector is the binding point in the shader.
             */
            struct Images {
                std::vector<Image> images;
            };

            /*! @brief Vertex attribute. All data need to be assigned by user.
             * @brief location is the location of the attribute in the shader.
             * @brief size is the number of components in the attribute.
             * @brief type is the data type of the attribute.
             * @brief normalized is whether the attribute is normalized.
             * @brief stride is the byte offset between consecutive attributes.
             * @brief offset is the byte offset of the first component of the attribute.
             */
            struct VertexAttrib {
                int location;
                int size;
                int type;
                bool normalized;
                int stride;
                int offset;
            };

            /*! @brief Attributes used in a pipeline.
             * @brief attribs is a vector of vertex attributes.
             */
            struct Attribs {
                std::vector<VertexAttrib> attribs;
            };

            /*! @brief A bundle of pipeline objects.
             * @brief pipeline is the pipeline object.
             * @brief shaders is the shaders object.
             * @brief attribs is the attribs object.
             * @brief buffers is the buffers object.
             * @brief images is the images object.
             */
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