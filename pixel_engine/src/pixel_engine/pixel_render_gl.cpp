#include "pixel_engine/pixel_render_gl/pixel_render_gl.h"

#include "pixel_engine/pixel_render_gl/systems.h"

using namespace pixel_engine::pixel_render_gl::components;
using namespace pixel_engine::pixel_render_gl::systems;

void pixel_engine::pixel_render_gl::PixelRenderGLPlugin::build(App& app) {
    app.add_system_main(PreStartup{}, create_pipeline, in_set(render_gl::RenderGLStartupSets::after_context_creation))
        .add_system_main(Render{}, draw);
}

void pixel_engine::pixel_render_gl::systems::create_pipeline(Command command, Resource<AssetServerGL> asset_server) {
    unsigned int uniform_buffer;
    glCreateBuffers(1, &uniform_buffer);
    command.spawn(
        PipelineBundle{
            .shaders{
                .vertex_shader = asset_server->load_shader("../assets/shaders/pixel/shader.vert", GL_VERTEX_SHADER),
                .fragment_shader = asset_server->load_shader("../assets/shaders/pixel/shader.frag", GL_FRAGMENT_SHADER),
            },
            .attribs{
                .attribs{
                    VertexAttrib{0, 3, GL_FLOAT, false, sizeof(Pixel), offsetof(Pixel, position)},
                    VertexAttrib{1, 4, GL_FLOAT, false, sizeof(Pixel), offsetof(Pixel, color)},
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
        PixelPipeline{});
}

void pixel_engine::pixel_render_gl::systems::draw(
    Query<Get<const Pixels, const PixelSize, const Transform>> pixels_query,
    Query<Get<const Transform, const OrthoProjection>, With<Camera2d>, Without<>> camera_query,
    Query<Get<Pipeline, Buffers, Images>, With<PixelPipeline, ProgramLinked>, Without<>> pipeline_query) {
    for (auto [pipeline, buffers, images] : pipeline_query.iter()) {
        for (auto [cam_trans, ortho_proj] : camera_query.iter()) {
            for (auto [pixels, size, transform] : pixels_query.iter()) {
                glUseProgram(pipeline.program);
                glBindBuffer(GL_ARRAY_BUFFER, pipeline.vertex_buffer.buffer);
                glBindVertexArray(pipeline.vertex_array);
                glm::mat4 proj;
                glm::mat4 view;
                glm::mat4 model;
                ortho_proj.get_matrix(&proj);
                cam_trans.get_matrix(&view);
                transform.get_matrix(&model);
                buffers.uniform_buffers[0].write(&proj, sizeof(glm::mat4), 0);
                buffers.uniform_buffers[0].write(&view, sizeof(glm::mat4), sizeof(glm::mat4));
                buffers.uniform_buffers[0].write(&model, sizeof(glm::mat4), 2 * sizeof(glm::mat4));

                int index = 0;
                for (auto& buffer : buffers.uniform_buffers) {
                    glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.buffer);
                    int size;
                    glGetNamedBufferParameteriv(buffer.buffer, GL_BUFFER_SIZE, &size);
                    if (size < buffer.data.size()) {
                        glNamedBufferData(buffer.buffer, buffer.data.size(), buffer.data.data(), GL_DYNAMIC_DRAW);
                    } else {
                        glNamedBufferSubData(buffer.buffer, 0, buffer.data.size(), buffer.data.data());
                    }
                    index++;
                }

                int count = 0;
                float size_x = size.width;
                float size_y = size.height;
                for (auto& pixel : pixels.pixels) {
                    float x = pixel.position[0];
                    float y = pixel.position[1];
                    float z = pixel.position[2];
                    float color[4] = {pixel.color[0], pixel.color[1], pixel.color[2], pixel.color[3]};
                    Pixel vertices[] = {
                        {{x, y, z}, {color[0], color[1], color[2], color[3]}},
                        {{x + size_x, y, z}, {color[0], color[1], color[2], color[3]}},
                        {{x + size_x, y + size_y, z}, {color[0], color[1], color[2], color[3]}},
                        {{x, y, z}, {color[0], color[1], color[2], color[3]}},
                        {{x + size_x, y + size_y, z}, {color[0], color[1], color[2], color[3]}},
                        {{x, y + size_y, z}, {color[0], color[1], color[2], color[3]}},
                    };
                    pipeline.vertex_buffer.write(vertices, sizeof(vertices), count * sizeof(Pixel));
                    count += 6;
                }

                int vertex_buffer_size;
                glGetNamedBufferParameteriv(pipeline.vertex_buffer.buffer, GL_BUFFER_SIZE, &vertex_buffer_size);
                if (vertex_buffer_size < pixels.pixels.size() * 6 * sizeof(Pixel)) {
                    glNamedBufferData(
                        pipeline.vertex_buffer.buffer, pixels.pixels.size() * 6 * sizeof(Pixel),
                        pipeline.vertex_buffer.data.data(), GL_DYNAMIC_DRAW);
                } else {
                    glNamedBufferSubData(
                        pipeline.vertex_buffer.buffer, 0, pixels.pixels.size() * 6 * sizeof(Pixel),
                        pipeline.vertex_buffer.data.data());
                }
                glDrawArrays(GL_TRIANGLES, 0, count);
            }
            return;
        }
    }
}