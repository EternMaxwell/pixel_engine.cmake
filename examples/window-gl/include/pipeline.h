#pragma once

#include <pixel_engine/plugins/asset_server_gl.h>
#include <pixel_engine/plugins/render_gl.h>
#include <pixel_engine/prelude.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace pipeline_test {
    using namespace pixel_engine;
    using namespace prelude;
    using namespace plugins::render_gl;
    using namespace plugins::asset_server_gl;

    struct TestPipeline {};

#pragma pack(push, 1)
    struct Vertex {
        float position[3];
        float color[3];
        float tex_coords[2];
    };
#pragma pack(pop)

    void create_pipeline(Command command, Resource<AssetServerGL> asset_server) {
        spdlog::debug("Creating pipeline");
        unsigned int uniform_buffer;
        glCreateBuffers(1, &uniform_buffer);
        command.spawn(
            PipelineBundle{
                .shaders{
                    .vertex_shader = asset_server->load_shader("../assets/shaders/shader.vert", GL_VERTEX_SHADER),
                    .fragment_shader = asset_server->load_shader("../assets/shaders/shader.frag", GL_FRAGMENT_SHADER)},
                .attribs{
                    .attribs = {VertexAttrib{0, 3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, position)},
                                VertexAttrib{1, 3, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, color)},
                                VertexAttrib{2, 2, GL_FLOAT, false, sizeof(Vertex), offsetof(Vertex, tex_coords)}}},
                .buffers{.uniform_buffers = {{.buffer = uniform_buffer}}},
                .images{.images = {Image{
                            .texture = asset_server->load_image_2d("../assets/textures/test.png"),
                            .sampler = asset_server->create_sampler(GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST)}}}},
            TestPipeline{});
    }

    void draw(Query<std::tuple<Pipeline, const TestPipeline, const ProgramLinked, Buffers>, std::tuple<>> query,
              Query<std::tuple<const plugins::window_gl::WindowHandle, const plugins::window_gl::PrimaryWindow,
                               const plugins::window_gl::WindowSize>,
                    std::tuple<>>
                  window_query) {
        for (auto [window_handle, window_size] : window_query.iter()) {
            for (auto [pipeline, buffers] : query.iter()) {
                Vertex vertices[] = {{{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}},
                                     {{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1, 0}},
                                     {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1, 1}},
                                     {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1, 1}},
                                     {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 1}},
                                     {{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}}};
                float aspect = (float)window_size.width / (float)window_size.height;
                glm::mat4 projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
                glm::mat4 view = glm::mat4(1.0f);
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

    class PipelineTestPlugin : public Plugin {
       public:
        void build(App& app) override {
            SystemNode draw_node;

            app.add_system_main(Startup{}, create_pipeline,
                                after(app.get_plugin<WindowGLPlugin>()->start_up_window_create_node))
                .add_system(Render{}, draw, &draw_node)
                .add_system_main(Render{}, bind_pipeline<TestPipeline, PrimaryWindow>, after(draw_node));
        }
    };
}  // namespace pipeline_test