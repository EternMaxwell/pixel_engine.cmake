#include "pixel_engine/pixel_render_gl/pixel_render_gl.h"
#include "pixel_engine/pixel_render_gl/systems.h"

using namespace pixel_engine::pixel_render_gl::components;
using namespace pixel_engine::pixel_render_gl::systems;

namespace pixel_shader_source {
const char* vertex_shader = R"(#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 outColor;

layout(binding = 0, std140) uniform UniformBufferObject {
    mat4 proj;
    mat4 view;
    mat4 model;
};

void main() {
    mat4 mvp = proj * view * model;
    gl_Position = mvp * vec4(position, 1.0);
    outColor = color;
})";

const char* fragment_shader = R"(#version 450 core

layout(location = 0) in vec4 color;

layout(location = 0) out vec4 fragColor;

void main() { fragColor = color; })";
}  // namespace pixel_shader_source

EPIX_API void pixel_engine::pixel_render_gl::PixelRenderGLPlugin::build(App& app
) {
    app.add_system(PreStartup, create_pipeline)
        .in_set(render_gl::RenderGLStartupSets::after_context_creation)
        .use_worker("single")
        ->add_system(Render, draw)
        .use_worker("single");
}

EPIX_API void pixel_engine::pixel_render_gl::systems::create_pipeline(
    Command command, ResMut<AssetServerGL> asset_server
) {
    unsigned int uniform_buffer;
    glCreateBuffers(1, &uniform_buffer);
    ShaderPtr vertex_shader;
    vertex_shader.create(GL_VERTEX_SHADER);
    vertex_shader.source(pixel_shader_source::vertex_shader);
    vertex_shader.compile();
    ShaderPtr fragment_shader;
    fragment_shader.create(GL_FRAGMENT_SHADER);
    fragment_shader.source(pixel_shader_source::fragment_shader);
    fragment_shader.compile();
    command.spawn(
        PipelineCreationBundle{
            .shaders{
                .vertex_shader = vertex_shader,
                .fragment_shader = fragment_shader
            },
            .attribs{{
                {0, 3, GL_FLOAT, GL_FALSE, sizeof(Pixel),
                 offsetof(Pixel, position)},
                {1, 4, GL_FLOAT, GL_FALSE, sizeof(Pixel), offsetof(Pixel, color)
                },
            }}
        },
        PixelPipeline{}
    );
}

EPIX_API void pixel_engine::pixel_render_gl::systems::draw(
    Query<Get<const Pixels, const PixelSize, const Transform>> pixels_query,
    Query<
        Get<const Transform, const OrthoProjection>,
        With<Camera2d>,
        Without<>> camera_query,
    render_gl::PipelineQuery::query_type<PixelPipeline> pipeline_query
) {
    if (!pipeline_query.single().has_value()) return;
    if (!camera_query.single().has_value()) return;
    auto
        [pipeline_layout, program, view_port, depth_range,
         per_sample_operations, frame_buffer] = pipeline_query.single().value();
    auto [camera_transform, ortho_projection] = camera_query.single().value();
    if (!pipeline_layout.uniform_buffers[0].valid()) {
        pipeline_layout.uniform_buffers[0].create();
        pipeline_layout.uniform_buffers[0].data(
            NULL, 3 * sizeof(glm::mat4), GL_DYNAMIC_DRAW
        );
    }

    program.use();
    // view_port.use();
    depth_range.use();
    per_sample_operations.use();
    frame_buffer.bind();

    glm::mat4 proj, view;
    camera_transform.get_matrix(&view);
    ortho_projection.get_matrix(&proj);
    pipeline_layout.uniform_buffers[0].subData(&proj, sizeof(glm::mat4), 0);
    pipeline_layout.uniform_buffers[0].subData(
        &view, sizeof(glm::mat4), sizeof(glm::mat4)
    );
    std::vector<Pixel> vertices;
    for (auto [pixels, pixel_size, transform] : pixels_query.iter()) {
        for (auto& per_pix : pixels.pixels) {
            vertices.push_back(
                {{per_pix.position[0], per_pix.position[1], per_pix.position[2]
                 },
                 {per_pix.color[0], per_pix.color[1], per_pix.color[2],
                  per_pix.color[3]}}
            );
            vertices.push_back(
                {{per_pix.position[0] + pixel_size.width, per_pix.position[1],
                  per_pix.position[2]},
                 {per_pix.color[0], per_pix.color[1], per_pix.color[2],
                  per_pix.color[3]}}
            );
            vertices.push_back(
                {{per_pix.position[0] + pixel_size.width,
                  per_pix.position[1] + pixel_size.height, per_pix.position[2]},
                 {per_pix.color[0], per_pix.color[1], per_pix.color[2],
                  per_pix.color[3]}}
            );
            vertices.push_back(
                {{per_pix.position[0], per_pix.position[1], per_pix.position[2]
                 },
                 {per_pix.color[0], per_pix.color[1], per_pix.color[2],
                  per_pix.color[3]}}
            );
            vertices.push_back(
                {{per_pix.position[0] + pixel_size.width,
                  per_pix.position[1] + pixel_size.height, per_pix.position[2]},
                 {per_pix.color[0], per_pix.color[1], per_pix.color[2],
                  per_pix.color[3]}}
            );
            vertices.push_back(
                {{per_pix.position[0], per_pix.position[1] + pixel_size.height,
                  per_pix.position[2]},
                 {per_pix.color[0], per_pix.color[1], per_pix.color[2],
                  per_pix.color[3]}}
            );
        }
        pipeline_layout.vertex_buffer.data(
            vertices.data(), sizeof(Pixel) * vertices.size(), GL_DYNAMIC_DRAW
        );
        glm::mat4 model;
        transform.get_matrix(&model);
        pipeline_layout.uniform_buffers[0].subData(
            &model, sizeof(glm::mat4), 2 * sizeof(glm::mat4)
        );

        pipeline_layout.use();

        glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        vertices.clear();
    }
}