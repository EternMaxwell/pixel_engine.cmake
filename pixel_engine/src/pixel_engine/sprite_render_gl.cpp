#include "pixel_engine/sprite_render_gl/sprite_render.h"
#include "pixel_engine/sprite_render_gl/systems.h"

using namespace pixel_engine::sprite_render_gl::components;
using namespace pixel_engine::sprite_render_gl::systems;
using namespace pixel_engine::prelude;

void pixel_engine::sprite_render_gl::SpriteRenderGLPlugin::build(App& app) {
    app.add_system(Startup, create_pipeline)
        .use_worker("single")
        ->configure_sets(
            SpriteRenderGLSets::before_draw, SpriteRenderGLSets::draw,
            SpriteRenderGLSets::after_draw
        )
        .add_system(Render, draw_sprite)
        .in_set(SpriteRenderGLSets::draw)
        .use_worker("single");
}

void pixel_engine::sprite_render_gl::systems::create_pipeline(
    Command command, ResMut<AssetServerGL> asset_server
) {
    ShaderPtr vertex_shader;
    vertex_shader = asset_server->load_shader(
        "../assets/shaders/sprite/shader.vert", GL_VERTEX_SHADER
    );
    ShaderPtr fragment_shader;
    fragment_shader = asset_server->load_shader(
        "../assets/shaders/sprite/shader.frag", GL_FRAGMENT_SHADER
    );
    command.spawn(
        PipelineCreationBundle{
            .shaders{
                .vertex_shader = vertex_shader,
                .fragment_shader = fragment_shader,
            },
            .attribs{{
                {0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0},
                {1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 3 * sizeof(float)},
                {2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 7 * sizeof(float)},
            }},
        },
        SpritePipeline{}
    );
}

void pixel_engine::sprite_render_gl::components::Sprite::vertex_data(
    Vertex* vertices
) const {
    std::memcpy(&vertices[0].color, &color, 4 * sizeof(float));
    std::memcpy(&vertices[1].color, &color, 4 * sizeof(float));
    std::memcpy(&vertices[2].color, &color, 4 * sizeof(float));
    std::memcpy(&vertices[3].color, &color, 4 * sizeof(float));
    vertices[0].tex_coords[0] = 0.0f;
    vertices[0].tex_coords[1] = 0.0f;
    vertices[1].tex_coords[0] = 1.0f;
    vertices[1].tex_coords[1] = 0.0f;
    vertices[2].tex_coords[0] = 1.0f;
    vertices[2].tex_coords[1] = 1.0f;
    vertices[3].tex_coords[0] = 0.0f;
    vertices[3].tex_coords[1] = 1.0f;
    vertices[0].position[0] = -center[0] * size[0];
    vertices[0].position[1] = -center[1] * size[1];
    vertices[0].position[2] = 0.0f;
    vertices[1].position[0] = (1.0f - center[0]) * size[0];
    vertices[1].position[1] = -center[1] * size[1];
    vertices[1].position[2] = 0.0f;
    vertices[2].position[0] = (1.0f - center[0]) * size[0];
    vertices[2].position[1] = (1.0f - center[1]) * size[1];
    vertices[2].position[2] = 0.0f;
    vertices[3].position[0] = -center[0] * size[0];
    vertices[3].position[1] = (1.0f - center[1]) * size[1];
    vertices[3].position[2] = 0.0f;
}

void pixel_engine::sprite_render_gl::systems::draw_sprite(
    render_gl::PipelineQuery::query_type<SpritePipeline> pipeline_query,
    Query<
        Get<const Transform, const OrthoProjection>,
        With<Camera2d>,
        Without<>> camera_query,
    sprite_query_type sprite_query
) {
    if (!pipeline_query.single().has_value()) return;
    if (!camera_query.single().has_value()) return;
    auto
        [pipeline_layout, program, view_port, depth_range,
         per_sample_operations, frame_buffer] = pipeline_query.single().value();
    auto [camera_transform, ortho_projection] = camera_query.single().value();
    if (!pipeline_layout.uniform_buffers[0].valid())
        pipeline_layout.uniform_buffers[0].create();
    program.use();
    // view_port.use();
    depth_range.use();
    per_sample_operations.use();
    frame_buffer.bind();
    glm::mat4 view, proj;
    camera_transform.get_matrix(&view);
    ortho_projection.get_matrix(&proj);
    pipeline_layout.uniform_buffers[0].data(
        NULL, 3 * sizeof(glm::mat4), GL_DYNAMIC_DRAW
    );
    pipeline_layout.uniform_buffers[0].subData(
        glm::value_ptr(proj), sizeof(glm::mat4), 0
    );
    pipeline_layout.uniform_buffers[0].subData(
        glm::value_ptr(view), sizeof(glm::mat4), sizeof(glm::mat4)
    );
    Vertex vertices[4];
    pipeline_layout.vertex_buffer.data(
        NULL, 4 * sizeof(Vertex), GL_DYNAMIC_DRAW
    );
    for (auto [transform, sprite] : sprite_query.iter()) {
        glm::mat4 model;
        transform.get_matrix(&model);
        pipeline_layout.uniform_buffers[0].subData(
            glm::value_ptr(model), sizeof(glm::mat4), 2 * sizeof(glm::mat4)
        );
        sprite.vertex_data(vertices);
        pipeline_layout.vertex_buffer.subData(vertices, 4 * sizeof(Vertex), 0);
        pipeline_layout.textures[0] = sprite.texture;
        pipeline_layout.use();
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}