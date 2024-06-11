#pragma once

#include <glad/gl.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace plugins {
        namespace render_gl {
            struct Pipeline {
                int program = 0;
                unsigned int vertex_array = 0;
            };

            struct Shaders {
                int vertex_shader = 0;
                int fragment_shader = 0;
                int geometry_shader = 0;
                int tess_control_shader = 0;
                int tess_evaluation_shader = 0;
            };

            struct Buffers {
                std::vector<int> uniform_buffers;
                std::vector<int> storage_buffers;
            };

            struct Images {
                std::vector<std::pair<int, int>> images;
            };

            struct VertexAttrib {
                int location;
                int size;
                int type;
                bool normalized;
                int stride;
                int offset;
            };

            struct Attribs {
                std::vector<VertexAttrib> attribs;
            };

            struct PipelineBundle {
                Bundle bundle;
                Pipeline pipeline;
                Shaders shaders;
                Attribs attribs;
                Buffers buffers;
                Images images;
            };

            struct ProgramLinked {};

            void create_pipelines(
                Command command,
                Query<std::tuple<entt::entity, Pipeline, Shaders, Attribs>, std::tuple<ProgramLinked>> query) {
                for (auto [id, pipeline, shaders, attribs] : query.iter()) {
                    spdlog::debug("Creating new pipeline");
                    pipeline.program = glCreateProgram();
                    glCreateVertexArrays(1, &pipeline.vertex_array);
                    if (shaders.vertex_shader) {
                        glAttachShader(pipeline.program, shaders.vertex_shader);
                    }
                    if (shaders.fragment_shader) {
                        glAttachShader(pipeline.program, shaders.fragment_shader);
                    }
                    if (shaders.geometry_shader) {
                        glAttachShader(pipeline.program, shaders.geometry_shader);
                    }
                    if (shaders.tess_control_shader) {
                        glAttachShader(pipeline.program, shaders.tess_control_shader);
                    }
                    if (shaders.tess_evaluation_shader) {
                        glAttachShader(pipeline.program, shaders.tess_evaluation_shader);
                    }
                    for (auto& attrib : attribs.attribs) {
                        glVertexAttribPointer(attrib.location, attrib.size, attrib.type, attrib.normalized,
                                              attrib.stride, (void*)attrib.offset);
                        glEnableVertexAttribArray(attrib.location);
                    }
                    glLinkProgram(pipeline.program);
                    int success;
                    glGetProgramiv(pipeline.program, GL_LINK_STATUS, &success);
                    if (!success) {
                        char info_log[512];
                        glGetProgramInfoLog(pipeline.program, 512, NULL, info_log);
                        throw std::runtime_error("Failed to link program: " + std::string(info_log));
                    }
                    command.entity(id).emplace(ProgramLinked{});
                    spdlog::debug("End creating new pipeline.");
                }
            }
        }  // namespace render_gl

        class RenderGLPlugin : public entity::Plugin {
           public:
            Node create_pipelines_node;

            void build(App& app) override {
                using namespace render_gl;
                app.add_system_main(PreRender{}, create_pipelines, &create_pipelines_node);
            }
        };
    }  // namespace plugins
}  // namespace pixel_engine