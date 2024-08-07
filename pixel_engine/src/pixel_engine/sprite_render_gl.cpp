#include "pixel_engine/sprite_render_gl/sprite_render.h"
#include "pixel_engine/sprite_render_gl/systems.h"

using namespace pixel_engine::sprite_render_gl::components;
using namespace pixel_engine::sprite_render_gl::systems;

void pixel_engine::sprite_render_gl::SpriteRenderGLPlugin::build(App& app) {
    app.add_system_main(Startup{}, create_pipeline)
        .configure_sets(
            SpriteRenderGLSets::before_draw, SpriteRenderGLSets::draw,
            SpriteRenderGLSets::after_draw)
        .add_system_main(Render{}, draw, in_set(SpriteRenderGLSets::draw));
}

void pixel_engine::sprite_render_gl::systems::create_pipeline(
    Command command, Resource<AssetServerGL> asset_server) {
    pipeline::ShaderPtr vertex_shader;
    vertex_shader.id = asset_server->load_shader(
        "../assets/shaders/sprite/shader.vert", GL_VERTEX_SHADER);
    pipeline::ShaderPtr fragment_shader;
    fragment_shader.id = asset_server->load_shader(
        "../assets/shaders/sprite/shader.frag", GL_FRAGMENT_SHADER);
    command.spawn(
        pipeline::PipelineCreationBundle{
            .shaders =
                {
                    .vertex_shader = vertex_shader,
                    .fragment_shader = fragment_shader,
                },
            .attribs{{
                {0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0},
                {1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 3 * sizeof(float)},
                {2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 7 * sizeof(float)},
            }},
        },
        SpritePipeline{});
}

void pixel_engine::sprite_render_gl::systems::create_render_pass(
    Command command,
    Query<
        Get<entt::entity, Sprite>, With<Transform>,
        Without<pipeline::RenderPass>>
        sprite_query,
    Query<Get<entt::entity>, With<SpritePipeline>> pipeline_query) {
    if (!pipeline_query.single().has_value()) {
        return;
    }
    auto [pipeline_entity] = pipeline_query.single().value();
    for (auto [entity, sprite] : sprite_query.iter()) {
        pipeline::TextureBindings texture_bindings;
        texture_bindings.set(
            0, pipeline::Image{
                   .texture{
                       .id = sprite.texture,
                   },
                   .sampler{
                       .id = sprite.sampler,
                   },
               });
        pipeline::UniformBufferBindings uniform_buffer_bindings;
        pipeline::BufferPtr uniform_buffer;
        uniform_buffer.create();
        uniform_buffer_bindings.set(0, uniform_buffer);
        auto layout = command.spawn(pipeline::PipelineLayoutBundle{
            .uniform_buffers = uniform_buffer_bindings,
            .textures = texture_bindings,
        });
        command.entity(entity).emplace(pipeline::RenderPassBundle{
            .pipeline{pipeline_entity},
            .layout{layout},
        });
    }
}

void pixel_engine::sprite_render_gl::systems::draw(
    Query<Get<const Sprite, const Transform>> sprite_query,
    Query<
        Get<const Transform, const OrthoProjection>, With<Camera2d>, Without<>>
        camera_query,
    Query<
        Get<const Pipeline, Buffers, Images>,
        With<SpritePipeline, ProgramLinked>, Without<>>
        pipeline_query) {
    for (auto [pipeline, buffers, images] : pipeline_query.iter()) {
        for (auto [camera_transform, projection] : camera_query.iter()) {
            for (auto [sprite, transform] : sprite_query.iter()) {
                Vertex vertices[] = {
                    {{-sprite.size[0] / 2, -sprite.size[1] / 2, 0.0f},
                     {sprite.color[0], sprite.color[1], sprite.color[2],
                      sprite.color[3]},
                     {0.0f, 0.0f}},
                    {{sprite.size[0] / 2, -sprite.size[1] / 2, 0.0f},
                     {sprite.color[0], sprite.color[1], sprite.color[2],
                      sprite.color[3]},
                     {1.0f, 0.0f}},
                    {{sprite.size[0] / 2, sprite.size[1] / 2, 0.0f},
                     {sprite.color[0], sprite.color[1], sprite.color[2],
                      sprite.color[3]},
                     {1.0f, 1.0f}},
                    {{-sprite.size[0] / 2, sprite.size[1] / 2, 0.0f},
                     {sprite.color[0], sprite.color[1], sprite.color[2],
                      sprite.color[3]},
                     {0.0f, 1.0f}},
                };
                glNamedBufferData(
                    pipeline.vertex_buffer.buffer, sizeof(vertices), vertices,
                    GL_DYNAMIC_DRAW);
                glUseProgram(pipeline.program);
                glBindBuffer(GL_ARRAY_BUFFER, pipeline.vertex_buffer.buffer);
                glBindVertexArray(pipeline.vertex_array);
                images.images[0].texture = sprite.texture;
                images.images[0].sampler = sprite.sampler;
                glm::mat4 model, view, proj;
                projection.get_matrix(&proj);
                camera_transform.get_matrix(&view);
                transform.get_matrix(&model);
                buffers.uniform_buffers[0].write(
                    glm::value_ptr(proj), sizeof(glm::mat4), 0);
                buffers.uniform_buffers[0].write(
                    glm::value_ptr(view), sizeof(glm::mat4), sizeof(glm::mat4));
                buffers.uniform_buffers[0].write(
                    glm::value_ptr(model), sizeof(glm::mat4),
                    2 * sizeof(glm::mat4));
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
                    glGetNamedBufferParameteriv(
                        buffer.buffer, GL_BUFFER_SIZE, &size);
                    if (size < buffer.data.size()) {
                        glNamedBufferData(
                            buffer.buffer, buffer.data.size(),
                            buffer.data.data(), GL_DYNAMIC_DRAW);
                    } else {
                        glNamedBufferSubData(
                            buffer.buffer, 0, buffer.data.size(),
                            buffer.data.data());
                    }
                    index++;
                }
                index = 0;
                for (auto& buffer : buffers.storage_buffers) {
                    glBindBufferBase(
                        GL_SHADER_STORAGE_BUFFER, index, buffer.buffer);
                    int size;
                    glGetNamedBufferParameteriv(
                        buffer.buffer, GL_BUFFER_SIZE, &size);
                    if (size < buffer.data.size()) {
                        glNamedBufferData(
                            buffer.buffer, buffer.data.size(),
                            buffer.data.data(), GL_DYNAMIC_DRAW);
                    } else {
                        glNamedBufferSubData(
                            buffer.buffer, 0, buffer.data.size(),
                            buffer.data.data());
                    }
                    index++;
                }
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }
            return;
        }
    }
}