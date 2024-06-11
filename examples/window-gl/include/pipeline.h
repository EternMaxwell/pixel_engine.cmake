#pragma once

#include <pixel_engine/plugins/asset_server_gl.h>
#include <pixel_engine/plugins/render_gl.h>
#include <pixel_engine/prelude.h>

namespace pipeline_test {
    using namespace pixel_engine;
    using namespace prelude;
    using namespace plugins::render_gl;
    using namespace plugins::asset_server_gl;

    struct TestPipeline {};

    void create_pipeline(Command command, Resource<AssetServerGL> asset_server) {
        spdlog::debug("Creating pipeline");
        command.spawn(
            PipelineBundle{
                .shaders{
                    .vertex_shader = asset_server->load_shader("./assets/shaders/shader.vert", GL_VERTEX_SHADER),
                    .fragment_shader = asset_server->load_shader("./assets/shaders/shader.frag", GL_FRAGMENT_SHADER)},
                .attribs{.attribs = {VertexAttrib{0, 3, GL_FLOAT, false, 0, 0},
                                     VertexAttrib{0, 3, GL_FLOAT, false, 0, 3 * sizeof(float)}}}},
            TestPipeline{});
    }

    class PipelineTestPlugin : public Plugin {
       public:
        void build(App& app) override {
            app.add_system_main(Startup{}, create_pipeline,
                                after(app.get_plugin<WindowGLPlugin>()->start_up_window_create_node));
        }
    };
}  // namespace pipeline_test