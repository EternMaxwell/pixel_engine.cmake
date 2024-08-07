#include "pixel_engine/sprite_render_gl/sprite_render.h"
#include "pixel_engine/sprite_render_gl/systems.h"

using namespace pixel_engine::sprite_render_gl::components;
using namespace pixel_engine::sprite_render_gl::systems;

void pixel_engine::sprite_render_gl::SpriteRenderGLPlugin::build(App& app) {
    app.add_system_main(Startup{}, create_pipeline)
        .configure_sets(
            SpriteRenderGLSets::before_draw, SpriteRenderGLSets::draw,
            SpriteRenderGLSets::after_draw)
        .add_system_main(PreRender{}, create_render_pass)
        .add_system_main(
            Render{}, update_uniform, in_set(SpriteRenderGLSets::draw));
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
        pipeline::DrawArraysData draw_arrays_data;
        draw_arrays_data.write(vertices, sizeof(vertices), 0);
        command.entity(entity).spawn(
            pipeline::DrawArraysBundle{
                .render_pass{entity},
                .draw_arrays{
                    .mode = GL_TRIANGLE_FAN,
                    .first = 0,
                    .count = 4,
                },
            },
            draw_arrays_data);
    }
}

void pixel_engine::sprite_render_gl::systems::update_uniform(
    Query<
        Get<const Transform, const OrthoProjection>, With<Camera2d>, Without<>>
        camera_query,
    Query<
        Get<pipeline::PipelineLayoutPtr, const Transform>,
        With<SpritePipeline, Sprite>>
        layout_query,
    Query<Get<pipeline::UniformBufferBindings>, With<pipeline::PipelineLayout>>
        uniform_buffer_query) {
    for (auto [layout, transform] : layout_query.iter()) {
        for (auto [camera_transform, ortho_projection] : camera_query.iter()) {
            glm::mat4 proj;
            glm::mat4 view;
            glm::mat4 model;
            transform.get_matrix(&model);
            camera_transform.get_matrix(&view);
            ortho_projection.get_matrix(&proj);
            auto [uniform_buffer_bindings] =
                uniform_buffer_query.get(layout.layout);
            uniform_buffer_bindings.buffers[0].data(
                NULL, 3 * sizeof(glm::mat4), GL_DYNAMIC_DRAW);
            uniform_buffer_bindings.buffers[0].subData(
                &proj, sizeof(glm::mat4), 0);
            uniform_buffer_bindings.buffers[0].subData(
                &view, sizeof(glm::mat4), sizeof(glm::mat4));
            uniform_buffer_bindings.buffers[0].subData(
                &model, sizeof(glm::mat4), 2 * sizeof(glm::mat4));
        }
    }
}