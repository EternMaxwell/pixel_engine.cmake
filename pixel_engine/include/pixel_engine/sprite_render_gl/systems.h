#pragma once

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

#include "components.h"
#include "pixel_engine/asset_server_gl/resources.h"
#include "pixel_engine/entity.h"
#include "pixel_engine/render_gl/render_gl.h"

namespace pixel_engine {
    namespace sprite_render_gl {
        namespace systems {
            using namespace prelude;
            using namespace asset_server_gl::resources;
            using namespace render_gl::components;
            using namespace sprite_render_gl::components;

            void create_pipeline(Command command, Resource<AssetServerGL> asset_server) {
                unsigned int uniform_buffer;
                glCreateBuffers(1, &uniform_buffer);
                command.spawn(
                    PipelineBundle{
                        .shaders{
                            .vertex_shader =
                                asset_server->load_shader("../assets/shaders/sprite/shader.vert", GL_VERTEX_SHADER),
                            .fragment_shader =
                                asset_server->load_shader("../assets/shaders/sprite/shader.frag", GL_FRAGMENT_SHADER),
                        },
                        .attribs{
                            .attribs{
                                VertexAttrib{0, 3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, position)},
                                VertexAttrib{1, 4, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, color)},
                                VertexAttrib{2, 2, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, tex_coords)},
                            },
                        },
                        .buffers{
                            .uniform_buffers{{
                                .buffer = uniform_buffer,
                            }},
                        },
                        .images{
                            .images{Image{}},
                        },
                    },
                    SpritePipeline{});
            }

            void draw(
                Query<Get<const Sprite, const Transform>, Without<>> sprite_query,
                Query<Get<const Camera2d, const Transform, const OrthoProjection>, Without<>> camera_query,
                Query<Get<const Pipeline, Buffers, Images, const SpritePipeline, const ProgramLinked>, Without<>>
                    pipeline_query) {
                for (auto [pipeline, buffers, images] : pipeline_query.iter()) {
                    for (auto [camera_transform, projection] : camera_query.iter()) {
                        for (auto [sprite, transform] : sprite_query.iter()) {
                            Vertex vertices[] = {
                                {{-sprite.size[0] / 2, -sprite.size[1] / 2, 0.0f},
                                 {sprite.color[0], sprite.color[1], sprite.color[2], sprite.color[3]},
                                 {0.0f, 0.0f}},
                                {{sprite.size[0] / 2, -sprite.size[1] / 2, 0.0f},
                                 {sprite.color[0], sprite.color[1], sprite.color[2], sprite.color[3]},
                                 {1.0f, 0.0f}},
                                {{sprite.size[0] / 2, sprite.size[1] / 2, 0.0f},
                                 {sprite.color[0], sprite.color[1], sprite.color[2], sprite.color[3]},
                                 {1.0f, 1.0f}},
                                {{-sprite.size[0] / 2, sprite.size[1] / 2, 0.0f},
                                 {sprite.color[0], sprite.color[1], sprite.color[2], sprite.color[3]},
                                 {0.0f, 1.0f}},
                            };
                            glNamedBufferData(
                                pipeline.vertex_buffer.buffer, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
                            glUseProgram(pipeline.program);
                            glBindBuffer(GL_ARRAY_BUFFER, pipeline.vertex_buffer.buffer);
                            glBindVertexArray(pipeline.vertex_array);
                            images.images[0].texture = sprite.texture;
                            images.images[0].sampler = sprite.sampler;
                            glm::mat4 model, view, proj;
                            projection.get_matrix(&proj);
                            camera_transform.get_matrix(&view);
                            transform.get_matrix(&model);
                            buffers.uniform_buffers[0].write(glm::value_ptr(proj), sizeof(glm::mat4), 0);
                            buffers.uniform_buffers[0].write(
                                glm::value_ptr(view), sizeof(glm::mat4), sizeof(glm::mat4));
                            buffers.uniform_buffers[0].write(
                                glm::value_ptr(model), sizeof(glm::mat4), 2 * sizeof(glm::mat4));
                            int index = 0;
                            for (auto& image : images.images) {
                                glBindTextureUnit(index, image.texture);
                                glBindSampler(index, image.sampler);
                                index++;
                            }
                            index = 0;
                            for (auto& buffer : buffers.uniform_buffers) {
                                glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.buffer);
                                int size;
                                glGetNamedBufferParameteriv(buffer.buffer, GL_BUFFER_SIZE, &size);
                                if (size < buffer.data.size()) {
                                    glNamedBufferData(
                                        buffer.buffer, buffer.data.size(), buffer.data.data(), GL_DYNAMIC_DRAW);
                                } else {
                                    glNamedBufferSubData(buffer.buffer, 0, buffer.data.size(), buffer.data.data());
                                }
                                index++;
                            }
                            index = 0;
                            for (auto& buffer : buffers.storage_buffers) {
                                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.buffer);
                                int size;
                                glGetNamedBufferParameteriv(buffer.buffer, GL_BUFFER_SIZE, &size);
                                if (size < buffer.data.size()) {
                                    glNamedBufferData(
                                        buffer.buffer, buffer.data.size(), buffer.data.data(), GL_DYNAMIC_DRAW);
                                } else {
                                    glNamedBufferSubData(buffer.buffer, 0, buffer.data.size(), buffer.data.data());
                                }
                                index++;
                            }
                        }
                        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
                        return;
                    }
                }
            }
        }  // namespace systems
    }      // namespace sprite_render_gl
}  // namespace pixel_engine