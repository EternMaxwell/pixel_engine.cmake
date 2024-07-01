#include "pixel_engine/font_gl/font.h"
#include "pixel_engine/font_gl/systems.h"

using namespace pixel_engine::font_gl::components;
using namespace pixel_engine::font_gl::systems;
using namespace pixel_engine::font_gl::resources;

void pixel_engine::font_gl::FontGLPlugin::build(App& app) {
    app.configure_sets(FontGLSets::insert_library, FontGLSets::after_insertion)
        .add_system(PreStartup{}, insert_ft2_library, in_set(FontGLSets::insert_library))
        .add_system_main(PreStartup{}, create_pipeline, in_set(render_gl::RenderGLStartupSets::after_context_creation))
        .add_system_main(Render{}, draw);
}

void pixel_engine::font_gl::systems::insert_ft2_library(Command command) {
    FT2Library ft2_library;
    FT_Error error = FT_Init_FreeType(&ft2_library.library);
    if (error) {
        throw std::runtime_error("Failed to initialize FreeType library");
    }
    command.insert_resource(ft2_library);
}

void pixel_engine::font_gl::systems::create_pipeline(Command command, Resource<AssetServerGL> asset_server) {
    unsigned int uniform_buffer;
    glCreateBuffers(1, &uniform_buffer);
    unsigned int texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, GL_R32F, 4096, 512);
    command.spawn(
        PipelineBundle{
            .shaders{
                .vertex_shader = asset_server->load_shader("../assets/shaders/text/shader.vert", GL_VERTEX_SHADER),
                .fragment_shader = asset_server->load_shader("../assets/shaders/text/shader.frag", GL_FRAGMENT_SHADER),
            },
            .attribs{
                .attribs{
                    VertexAttrib{0, 3, GL_FLOAT, false, sizeof(TextVertex), offsetof(TextVertex, position)},
                    VertexAttrib{1, 4, GL_FLOAT, false, sizeof(TextVertex), offsetof(TextVertex, color)},
                    VertexAttrib{2, 2, GL_FLOAT, false, sizeof(TextVertex), offsetof(TextVertex, tex_coords)},
                },
            },
            .buffers{
                .uniform_buffers{{
                    .buffer = uniform_buffer,
                }},
            },
            .images{
                .images{Image{
                    .texture = texture,
                    .sampler = asset_server->create_sampler(GL_CLAMP, GL_CLAMP, GL_NEAREST, GL_NEAREST),
                }},
            },
        },
        TextPipeline{});
}

void pixel_engine::font_gl::systems::draw(
    Query<Get<const Text, const Transform>> text_query,
    Query<Get<const OrthoProjection>, With<Camera2d>, Without<>> camera_query,
    Query<Get<const window::WindowSize>, With<window::PrimaryWindow, window::WindowCreated>, Without<>> window_query,
    Query<Get<Pipeline, Buffers, Images>, With<TextPipeline, ProgramLinked>, Without<>> pipeline_query) {
    for (auto [pipeline, buffers, images] : pipeline_query.iter()) {
        for (auto [ortho_proj] : camera_query.iter()) {
            for (auto [window_size] : window_query.iter()) {
                for (auto [text, transform] : text_query.iter()) {
                    int err = FT_Set_Char_Size(text.font_face, 0, text.pixels, 1920, 1080);
                    if (err) {
                        continue;
                    }
                    err = FT_Set_Pixel_Sizes(text.font_face, 0, text.pixels);
                    if (err) {
                        continue;
                    }
                    FT_GlyphSlot slot = text.font_face->glyph;
                    int image_width, image_height;
                    glGetTextureLevelParameteriv(images.images[0].texture, 0, GL_TEXTURE_WIDTH, &image_width);
                    glGetTextureLevelParameteriv(images.images[0].texture, 0, GL_TEXTURE_HEIGHT, &image_height);
                    int x = 0, y = image_height;
                    int max_x = 0, min_y = image_height;
                    int lines = 1;
                    // glClearTexImage(images.images[0].texture, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
                    for (auto c : text.text) {
                        int glygh_index = FT_Get_Char_Index(text.font_face, c);
                        int err = FT_Load_Glyph(text.font_face, glygh_index, FT_LOAD_DEFAULT);
                        if (err) {
                            continue;
                        }
                        err = FT_Render_Glyph(
                            text.font_face->glyph, text.antialias ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO);
                        if (err) {
                            continue;
                        }
                        FT_Bitmap bitmap = slot->bitmap;

                        float* buffer = new float[bitmap.width * bitmap.rows];
                        for (int iy = 0; iy < bitmap.rows; iy++) {
                            for (int ix = 0; ix < bitmap.width; ix++) {
                                buffer[ix + iy * bitmap.width] =
                                    text.antialias
                                        ? bitmap.buffer[ix + iy * bitmap.pitch] / 255.0f
                                        : ((bitmap.buffer[(ix / 8 + iy * bitmap.pitch)] & (128 >> (ix & 7))) ? 1 : 0);
                            }
                        }
                        int full_width = x + bitmap.width;
                        int full_height = image_height - y + bitmap.rows;
                        if (full_height > image_height || full_width > image_width) {
                            unsigned int new_texture;
                            glCreateTextures(GL_TEXTURE_2D, 1, &new_texture);
                            glTextureStorage2D(
                                new_texture, 1, GL_R32F, std::max(full_width, image_width),
                                std::max(full_height, image_height));
                            glCopyImageSubData(
                                images.images[0].texture, GL_TEXTURE_2D, 0, 0, 0, 0, new_texture, GL_TEXTURE_2D, 0, 0,
                                0, 0, image_width, image_height, 1);
                            glDeleteTextures(1, &images.images[0].texture);
                            images.images[0].texture = new_texture;
                        }
                        glTextureSubImage2D(
                            images.images[0].texture, 0, x, y - bitmap.rows, bitmap.width, bitmap.rows, GL_RED,
                            GL_FLOAT, buffer);
                        delete buffer;

                        x += slot->advance.x >> 6;
                        max_x = x;
                        y -= slot->advance.y >> 6;
                        lines += (slot->advance.y >> 6) ? 1 : 0;
                        min_y = std::min(min_y, (int)(y - bitmap.rows));
                    }
                    glm::mat4 model;
                    transform.get_matrix(&model);
                    glm::mat4 proj;
                    ortho_proj.get_matrix(&proj);
                    float ratio = (x + 1) / ((float)(y - min_y + 1));
                    float height =
                        text.size ? text.size * lines
                                  : (ortho_proj.top - ortho_proj.bottom) * text.pixels * lines / window_size.height;
                    float s1 = 0;
                    float t1 = 1;
                    float s2 = max_x / 4096.0f;
                    float t2 = min_y / 512.0f;
                    float width = height * ratio;
                    float posx = -text.center[0] * width;
                    float posy = -text.center[1] * height;
                    TextVertex vertices[] = {
                        {{posx, posy, 0.0f}, {text.color[0], text.color[1], text.color[2], text.color[3]}, {s1, t1}},
                        {{posx + width, posy, 0.0f},
                         {text.color[0], text.color[1], text.color[2], text.color[3]},
                         {s2, t1}},
                        {{posx + width, posy + height, 0.0f},
                         {text.color[0], text.color[1], text.color[2], text.color[3]},
                         {s2, t2}},
                        {{posx, posy + height, 0.0f},
                         {text.color[0], text.color[1], text.color[2], text.color[3]},
                         {s1, t2}},
                    };
                    pipeline.vertex_buffer.write(vertices, sizeof(vertices), 0);
                    buffers.uniform_buffers[0].write(glm::value_ptr(proj), sizeof(glm::mat4), 0);
                    buffers.uniform_buffers[0].write(glm::value_ptr(model), sizeof(glm::mat4), sizeof(glm::mat4));

                    glNamedBufferData(pipeline.vertex_buffer.buffer, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
                    glUseProgram(pipeline.program);
                    glBindBuffer(GL_ARRAY_BUFFER, pipeline.vertex_buffer.buffer);
                    glBindVertexArray(pipeline.vertex_array);
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
                        glGetNamedBufferParameteriv(buffer.buffer, GL_BUFFER_SIZE, &size);
                        if (size < buffer.data.size()) {
                            glNamedBufferData(buffer.buffer, buffer.data.size(), buffer.data.data(), GL_DYNAMIC_DRAW);
                        } else {
                            glNamedBufferSubData(buffer.buffer, 0, buffer.data.size(), buffer.data.data());
                        }
                        index++;
                    }
                    index = 0;
                    for (auto& buffer : buffers.storage_buffers) {
                        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.buffer);
                        int size;
                        glGetNamedBufferParameteriv(buffer.buffer, GL_BUFFER_SIZE, &size);
                        if (size < buffer.data.size()) {
                            glNamedBufferData(buffer.buffer, buffer.data.size(), buffer.data.data(), GL_DYNAMIC_DRAW);
                        } else {
                            glNamedBufferSubData(buffer.buffer, 0, buffer.data.size(), buffer.data.data());
                        }
                        index++;
                    }
                    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
                }
                return;
            }
        }
    }
}