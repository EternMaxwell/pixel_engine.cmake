#pragma once

#include <pixel_engine/asset_server_gl/resources.h>
#include <pixel_engine/prelude.h>
#include <pixel_engine/render_gl/components.h>

#include <vector>

namespace path {
    using namespace pixel_engine::prelude;
    using namespace pixel_engine::asset_server_gl::resources;
    using namespace pixel_engine::render_gl::components;
    using namespace pixel_engine::camera;
    using namespace pixel_engine::transform;

    struct Voxel {
        float occupied = 0;
        float color[3] = {0.0f, 0.0f, 0.0f};
    };

    struct PathVertex {
        float position[2];
    };

    struct PathPipeline {};

    struct Voxel_World {
        glm::ivec4 size;
        std::vector<Voxel> voxels;
        Voxel& at(int x, int y, int z) {
            return voxels[x + static_cast<size_t>(y) * size.x + static_cast<size_t>(z) * size.x * size.y];
        }
        void set(int x, int y, int z, Voxel voxel) {
            voxels[x + static_cast<size_t>(y) * size.x + static_cast<size_t>(z) * size.x * size.y] = voxel;
        }
        void resize(int x, int y, int z) {
            size.x = x;
            size.y = y;
            size.z = z;
            voxels.resize(static_cast<size_t>(x) * y * z);
        }
        size_t dsize() { return voxels.size(); }
    };

    void create_pipeline(Command command, Resource<AssetServerGL> asset_server) {
        unsigned int uniform_buffer;
        glCreateBuffers(1, &uniform_buffer);
        unsigned int storage_buffer;
        glCreateBuffers(1, &storage_buffer);
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
                    .storage_buffers{{
                        .buffer = storage_buffer,
                    }},
                },
            },
            PathPipeline{});
    }

    void create_world(Command command) {
        Voxel_World world;
        world.resize(100, 100, 100);
        for (int x = 0; x < world.size.x; x++) {
            for (int y = 0; y < world.size.y; y++) {
                for (int z = 0; z < world.size.z; z++) {
                    if ((y == 0 || y == world.size.z - 1) && (x == 0 || x == world.size.x - 1) &&
                        (z == 0 || z == world.size.z - 1))
                        world.set(
                            x, y, z,
                            Voxel{
                                .occupied = 1,
                                .color{1.0f, 1.0f, 1.0f},
                            });
                    else
                        world.set(
                            x, y, z,
                            Voxel{
                                .occupied = 1,
                                .color{(float)x / world.size.x, (float)y / world.size.y, (float)z / world.size.z},
                            });
                }
            }
        }
        command.spawn(world);  // Amount of data is copied here, but what else can i do.
    }

    void upload_world_to_gpu(Query<Get<Buffers>, With<PathPipeline>> query, Query<Get<Voxel_World>> world_query) {
        auto w = world_query.single();
        auto bs = query.single();
        if (!w || !bs) {
            return;
        }
        auto [buffers] = bs.value();
        auto [world] = w.value();
        glNamedBufferData(buffers.storage_buffers[0].buffer, world.dsize() * sizeof(Voxel) + 16, NULL, GL_DYNAMIC_DRAW);
        glNamedBufferSubData(buffers.storage_buffers[0].buffer, 0, 16, &world.size);
        glNamedBufferSubData(buffers.storage_buffers[0].buffer, 16, world.dsize() * sizeof(Voxel), world.voxels.data());
    }

    void create_camera_path(Command command) {
        command.spawn(CameraPath{
            .position = {0.0f, 0.0f, -20.0f, 1.0},
            .direction = {0.0f, 0.0f, 1.0f, 1.0},
            .up = {0.0f, 1.0f, 0.0f, 1.0},
        });
    }

    void render_world(
        Query<Get<Pipeline, Buffers>, With<PathPipeline>> query, Query<Get<Voxel_World>> world_query,
        Query<Get<CameraPath>> camera_query) {
        auto w = world_query.single();
        auto bs = query.single();
        auto c = camera_query.single();
        if (!w || !bs || !c) {
            return;
        }
        auto [pipeline, buffers] = bs.value();
        auto [world] = w.value();
        auto [cam] = c.value();
        cam.position += glm::vec4(-0.0003f, 0.0003f, -0.0001f, 0.0f);
        cam.direction = glm::rotate(glm::mat4(1.0f), 0.001f, glm::vec3(0.0f, 1.0f, 0.0f)) * cam.direction;
        glUseProgram(pipeline.program);
        glBindVertexArray(pipeline.vertex_array);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, buffers.uniform_buffers[0].buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers.storage_buffers[0].buffer);
        buffers.uniform_buffers[0].write(&cam.position, sizeof(cam.position), 0);
        buffers.uniform_buffers[0].write(&cam.direction, sizeof(cam.direction), sizeof(cam.direction));
        buffers.uniform_buffers[0].write(&cam.up, sizeof(cam.up), sizeof(cam.direction) * 2);
        glNamedBufferData(
            buffers.uniform_buffers[0].buffer, 3 * sizeof(glm::vec4), buffers.uniform_buffers[0].data.data(),
            GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, pipeline.vertex_buffer.buffer);
        float vertices[] = {
            -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    class PathPlugin : public Plugin {
       public:
        void build(App& app) override {
            app.add_system(Startup{}, create_camera_path);
            app.add_system(Startup{}, create_world);
            app.add_system_main(Startup{}, create_pipeline);
            app.add_system_main(PostStartup{}, upload_world_to_gpu);
            app.add_system_main(Render{}, render_world);
        }
    };
}  // namespace path