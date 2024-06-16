#pragma once

#include <pixel_engine/plugins/asset_server_gl.h>
#include <pixel_engine/prelude.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace pipeline_test {
    using namespace pixel_engine;
    using namespace prelude;
    using namespace plugins::asset_server_gl;
    using namespace window::components;
    using namespace render_gl::systems;
    using namespace render_gl::components;

    struct TestPipeline {};

#pragma pack(push, 1)
    struct Vertex {
        float position[3];
        float color[3];
        float tex_coords[2];
    };
#pragma pack(pop)

    void insert_camera(Command command) {
        spdlog::debug("Inserting camera");
        command.spawn(core_components::Camera2dBundle{});
    }

    void create_pipeline(Command command, Resource<AssetServerGL> asset_server) {
        spdlog::debug("Creating pipeline");
        unsigned int uniform_buffer;
        glCreateBuffers(1, &uniform_buffer);
        command.spawn(
            PipelineBundle{
                .shaders{
                    .vertex_shader = asset_server->load_shader("../assets/shaders/shader.vert", GL_VERTEX_SHADER),
                    .fragment_shader = asset_server->load_shader("../assets/shaders/shader.frag", GL_FRAGMENT_SHADER)},
                .attribs{.attribs{
                    VertexAttrib{0, 3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, position)},
                    VertexAttrib{1, 3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, color)},
                    VertexAttrib{2, 2, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, tex_coords)}}},
                .buffers{.uniform_buffers{{.buffer = uniform_buffer}}},
                .images{.images{Image{
                    .texture = asset_server->load_image_2d("../assets/textures/test.png"),
                    .sampler = asset_server->create_sampler(GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST)}}}},
            TestPipeline{});
    }

    void camera_ortho_to_primary_window(
        Query<Get<core_components::Camera2d, core_components::OrthoProjection>, Without<>> camera_query,
        Query<Get<const WindowSize, const PrimaryWindow>, Without<>> window_query) {
        for (auto [projection] : camera_query.iter()) {
            for (auto [window_size] : window_query.iter()) {
                float ratio = (float)window_size.width / (float)window_size.height;
                projection.left = -ratio;
                projection.right = ratio;
                return;
            }
        }
    }

    void draw(
        Query<
            Get<const core_components::Camera2d, const core_components::Transform,
                const core_components::OrthoProjection>,
            Without<>>
            camera_query,
        Query<Get<Pipeline, const TestPipeline, const ProgramLinked, Buffers>, Without<>> query,
        Query<Get<const WindowHandle, const PrimaryWindow, const WindowSize>, Without<>> window_query) {
        for (auto [tranform, proj] : camera_query.iter()) {
            for (auto [window_handle, window_size] : window_query.iter()) {
                for (auto [pipeline, buffers] : query.iter()) {
                    Vertex vertices[] = {{{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}},
                                         {{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1, 0}},
                                         {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1, 1}},
                                         {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1, 1}},
                                         {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 1}},
                                         {{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}}};
                    float aspect = (float)window_size.width / (float)window_size.height;
                    glm::mat4 projection;
                    glm::mat4 view;
                    tranform.get_matrix(&view);
                    proj.get_matrix(&projection);
                    glm::mat4 model = glm::mat4(1.0f);
                    buffers.uniform_buffers[0].write(glm::value_ptr(projection), sizeof(glm::mat4), 0);
                    buffers.uniform_buffers[0].write(glm::value_ptr(view), sizeof(glm::mat4), sizeof(glm::mat4));
                    buffers.uniform_buffers[0].write(glm::value_ptr(model), sizeof(glm::mat4), 2 * sizeof(glm::mat4));
                    pipeline.vertex_buffer.write(vertices, sizeof(vertices), 0);
                    pipeline.vertex_count += sizeof(vertices) / sizeof(Vertex);
                    return;
                }
            }
        }
    }

    class PipelineTestPlugin : public Plugin {
       public:
        void build(App& app) override {
            SystemNode draw_node;

            app.add_system(Startup{}, insert_camera)
                .add_system_main(
                    Startup{}, create_pipeline,
                    after(app.get_plugin<render_gl::RenderGLPlugin>()->context_creation_node))
                .add_system(PreRender{}, camera_ortho_to_primary_window)
                .add_system(Render{}, draw, &draw_node)
                .add_system_main(Render{}, bind_pipeline<TestPipeline, PrimaryWindow>, after(draw_node));
        }
    };
}  // namespace pipeline_test