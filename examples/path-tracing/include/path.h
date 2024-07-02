#pragma once

#include <pixel_engine/asset_server_gl/resources.h>
#include <pixel_engine/prelude.h>
#include <pixel_engine/render_gl/components.h>

#include <vector>

namespace path {
    using namespace pixel_engine::prelude;
    using namespace pixel_engine::asset_server_gl::resources;
    using namespace pixel_engine::render_gl::components;

    struct Voxel {
        float color[3] = {1.0f, 1.0f, 1.0f};
    };

    struct PathVertex {
        float position[2];
    };

    struct PathPipeline {};

    struct Voxel_World {
        int width_x;
        int width_y;
        int width_z;
        std::vector<Voxel> voxels;
        Voxel& at(int x, int y, int z) {
            return voxels[x + static_cast<size_t>(y) * width_x + static_cast<size_t>(z) * width_x * width_y];
        }
        void set(int x, int y, int z, Voxel voxel) {
            voxels[x + static_cast<size_t>(y) * width_x + static_cast<size_t>(z) * width_x * width_y] = voxel;
        }
        void resize(int x, int y, int z) {
            width_x = x;
            width_y = y;
            width_z = z;
            voxels.resize(static_cast<size_t>(x) * y * z);
        }
    };

    void create_pipeline(Command command, Resource<AssetServerGL> asset_server) {
        unsigned int uniform_buffer;
        glCreateBuffers(1, &uniform_buffer);
        command.spawn(
            PipelineBundle{
                .shaders{
                    .vertex_shader =
                        asset_server->load_shader("../assets/shaders/voxel-path-tracing/shader.vert", GL_VERTEX_SHADER),
                    .fragment_shader = asset_server->load_shader(
                        "../assets/shaders/voxel-path-tracing/shader.frag", GL_FRAGMENT_SHADER),
                },
                .attribs{
                    .attribs{
                        VertexAttrib{0, 2, GL_FLOAT, false, sizeof(PathVertex), offsetof(PathVertex, position)},
                    },
                },
                .buffers{
                    .uniform_buffers{{
                        .buffer = uniform_buffer,
                    }},
                },
            },
            PathPipeline{});
    }

}  // namespace path