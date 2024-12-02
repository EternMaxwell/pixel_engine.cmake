#include <pixel_engine/prelude.h>
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <box2d/box2d.h>
#include <pixel_engine/imgui.h>
#include <pixel_engine/physics2d.h>
#include <stb_image.h>

#include <earcut.hpp>
#include <random>
#include <stack>

#include "fragment_shader.h"
#include "vertex_shader.h"


using namespace pixel_engine::prelude;
using namespace pixel_engine::window;
using namespace pixel_engine::render_vk::components;
using namespace pixel_engine::sprite::components;
using namespace pixel_engine::sprite::resources;

namespace vk_trial {
using namespace pixel_engine::physics2d::utils;

template <typename T>
T& value_at(std::vector<std::vector<T>>& vec, size_t index) {
    if (!vec.size()) throw std::out_of_range("vector is empty");
    auto cur = vec.begin();
    while (cur != vec.end()) {
        if (index < (*cur).size()) return (*cur)[index];
        index -= cur->size();
        cur++;
    }
    throw std::out_of_range("index out of range");
}

struct pixelbin {
    int width, height;
    std::vector<uint32_t> pixels;
    int column;

    pixelbin(int width, int height) : width(width), height(height) {
        column = width / 32 + (width % 32 ? 1 : 0);
        pixels.resize(column * height);
    }
    void set(int x, int y, bool value) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        int index = x / 32 + y * column;
        int bit   = x % 32;
        if (value)
            pixels[index] |= 1 << bit;
        else
            pixels[index] &= ~(1 << bit);
    }
    bool operator[](glm::ivec2 pos) const {
        if (pos.x < 0 || pos.x >= width || pos.y < 0 || pos.y >= height)
            return false;
        int index = pos.x / 32 + pos.y * column;
        int bit   = pos.x % 32;
        return pixels[index] & (1 << bit);
    }
    bool contains(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) return false;
        int index = x / 32 + y * column;
        int bit   = x % 32;
        return pixels[index] & (1 << bit);
    }
    glm::ivec2 size() const { return {width, height}; }
};

using PixelBlock = pixel_engine::render::pixel::components::PixelBlock;
struct PixelBlockWrapper {
    PixelBlock block;
    bool contains(int x, int y) const {
        if (x < 0 || x >= block.size.x || y < 0 || y >= block.size.y)
            return false;
        return block[{x, y}].a > 0.1f;
    }
    glm::ivec2 size() const { return block.size; }
    PixelBlockWrapper(int width, int height)
        : block(PixelBlock::create({width, height})) {}
};

void create_b2d_world(Command command) {
    b2WorldDef def = b2DefaultWorldDef();
    def.gravity    = {0.0f, -29.8f};
    auto world     = b2CreateWorld(&def);
    command.spawn(world);
}

void create_pixel_block_with_collision(
    Command command, Query<Get<b2WorldId>> world_query
) {
    if (!world_query.single().has_value()) return;
    auto [world] = world_query.single().value();
    auto m_block = PixelBlockWrapper(64, 64);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (int i = 0; i < m_block.size().x; i++) {
        for (int j = 0; j < m_block.size().y; j++) {
            m_block.block[{i, j}] = {
                (float)(dis(gen) / 255.0f), (float)(dis(gen) / 255.0f),
                (float)(dis(gen) / 255.0f), dis(gen) > 127 ? 0.2f : 0.0f
            };
        }
    }
    auto blocks_bin = split_if_multiple(binary_grid(m_block));
    std::vector<std::pair<PixelBlockWrapper, glm::ivec2>> blocks;
    for (auto& block : blocks_bin) {
        glm::ivec2 offset;
        auto shrinked = shrink(block, &offset);
        blocks.emplace_back(
            PixelBlockWrapper(shrinked.width, shrinked.height), offset
        );
        for (int i = 0; i < shrinked.width; i++) {
            for (int j = 0; j < shrinked.height; j++) {
                if (shrinked.contains(i, j)) {
                    blocks.back().first.block[{i, j}] =
                        m_block.block[offset + glm::ivec2(i, j)];
                }
            }
        }
    }
    for (auto&& [block, offset] : blocks) {
        auto outline = find_outline(block);
        outline.push_back(outline[0]);
        auto simplified = douglas_peucker(outline, 0.5f);
        simplified.pop_back();
        auto holes          = find_holes(block);
        auto earcut_polygon = std::vector<std::vector<glm::ivec2>>();
        earcut_polygon.emplace_back(std::move(simplified));
        for (auto& hole : holes) {
            hole.push_back(hole[0]);
            auto hole_simplified = douglas_peucker(hole, 0.8f);
            hole_simplified.pop_back();
            earcut_polygon.emplace_back(std::move(hole_simplified));
        }
        auto triangle_list = mapbox::earcut(earcut_polygon);
        // std::cout << "triangle amount: " << triangle_list.size() / 3
        //           << std::endl;
        // for (size_t i = 0; i < triangle_list.size(); i += 3) {
        //     for (int j = 2; j >= 0; j--) {
        //         std::cout << std::format(
        //             "({},{}) ",
        //             value_at(earcut_polygon, triangle_list[i + j]).x,
        //             value_at(earcut_polygon, triangle_list[i + j]).y
        //         );
        //     }
        //     std::cout << std::endl;
        // }
        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type      = b2BodyType::b2_dynamicBody;
        body_def.position  = b2Vec2(offset.x, offset.y) * 5 +
                            b2Vec2(-m_block.size().x / 2, 40) * 5;
        auto body = b2CreateBody(world, &body_def);
        for (size_t i = 0; i < triangle_list.size(); i += 3) {
            b2Vec2 points[3];
            for (int j = 2; j >= 0; j--) {
                points[j] = {
                    (float)value_at(earcut_polygon, triangle_list[i + j]).x * 5,
                    (float)value_at(earcut_polygon, triangle_list[i + j]).y * 5
                };
            }
            b2Hull hull           = b2ComputeHull(points, 3);
            b2Polygon polygon     = b2MakePolygon(&hull, 0.0f);
            b2ShapeDef shape_def  = b2DefaultShapeDef();
            shape_def.density     = 1.0f;
            shape_def.friction    = 0.6f;
            shape_def.restitution = 0.1f;
            b2CreatePolygonShape(body, &shape_def, &polygon);
        }
        command.spawn(body, block);
    }
}

void render_pixel_block(
    Query<Get<PixelBlockWrapper, b2BodyId>> query,
    Query<Get<Device, Swapchain, Queue>, With<RenderContext>> context_query,
    Query<Get<pixel_engine::render::pixel::components::PixelRenderer>>
        pixel_renderer_query,
    Query<Get<b2WorldId>> world_query
) {
    if (!context_query.single().has_value()) return;
    if (!pixel_renderer_query.single().has_value()) return;
    if (!world_query.single().has_value()) return;
    auto [b2_world]                  = world_query.single().value();
    auto [device, swap_chain, queue] = context_query.single().value();
    auto [pixel_renderer]            = pixel_renderer_query.single().value();
    pixel_engine::render::pixel::components::PixelUniformBuffer uniform_buffer;
    uniform_buffer.proj = glm::ortho(
        -(float)swap_chain.extent.width / 2.0f,
        (float)swap_chain.extent.width / 2.0f,
        (float)swap_chain.extent.height / 2.0f,
        -(float)swap_chain.extent.height / 2.0f, -1000.0f, 1000.0f
    );
    uniform_buffer.view = glm::mat4(1.0f);
    pixel_renderer.begin(device, swap_chain, queue, uniform_buffer);
    for (auto [block, body] : query.iter()) {
        if (!b2Body_IsValid(body)) continue;
        auto position = b2Body_GetPosition(body);
        auto rotation = b2Body_GetRotation(body);
        auto angle    = std::atan2(rotation.s, rotation.c);
        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), {position.x, position.y, 0.0f});
        model = glm::rotate(model, angle, {0.0f, 0.0f, 1.0f});
        model = glm::scale(model, {5.0f, 5.0f, 1.0f});
        pixel_renderer.set_model(model);
        for (int i = 0; i < block.size().x; i++) {
            for (int j = 0; j < block.size().y; j++) {
                auto& color = block.block[{i, j}];
                if (color.a == 0.0f) continue;
                pixel_renderer.draw(color, {i, j});
            }
        }
    }
    pixel_renderer.end();
}

void create_ground(Command command, Query<Get<b2WorldId>> world_query) {
    if (!world_query.single().has_value()) return;
    auto [world]         = world_query.single().value();
    b2BodyDef body_def   = b2DefaultBodyDef();
    body_def.type        = b2BodyType::b2_staticBody;
    body_def.position    = {0.0f, -100.0f};
    auto ground          = b2CreateBody(world, &body_def);
    b2Polygon ground_box = b2MakeBox(1000.0f, 10.0f);
    b2ShapeDef shape_def = b2DefaultShapeDef();
    b2CreatePolygonShape(ground, &shape_def, &ground_box);
    command.spawn(ground);
}

void create_dynamic(Command command, Query<Get<b2WorldId>> world_query) {
    if (!world_query.single().has_value()) return;
    auto [world]          = world_query.single().value();
    b2BodyDef body_def    = b2DefaultBodyDef();
    body_def.type         = b2BodyType::b2_dynamicBody;
    body_def.position     = {0.0f, 100.0f};
    auto body             = b2CreateBody(world, &body_def);
    b2Polygon box         = b2MakeBox(4.0f, 4.0f);
    b2ShapeDef shape_def  = b2DefaultShapeDef();
    shape_def.density     = 1.0f;
    shape_def.friction    = 0.1f;
    shape_def.restitution = 0.9f;
    b2CreatePolygonShape(body, &shape_def, &box);
    command.spawn(body);
}

using namespace pixel_engine::input;

void create_dynamic_from_click(
    Command command,
    Query<Get<b2WorldId>> world_query,
    Query<
        Get<const Window,
            const ButtonInput<MouseButton>,
            const ButtonInput<KeyCode>>> window_query
) {
    if (!world_query.single().has_value()) return;
    if (!window_query.single().has_value()) return;
    auto [window, mouse_input, key_input] = window_query.single().value();
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (!mouse_input.pressed(pixel_engine::input::MouseButton1) &&
        !key_input.just_pressed(pixel_engine::input::KeySpace))
        return;
    auto [world]       = world_query.single().value();
    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type      = b2BodyType::b2_dynamicBody;
    body_def.position  = {
        (float)window.get_cursor().x - (float)window.get_size().width / 2.0f,
        -(float)window.get_cursor().y + (float)window.get_size().height / 2.0f
    };
    spdlog::info(
        "Creating body at {}, {}", body_def.position.x, body_def.position.y
    );
    auto body             = b2CreateBody(world, &body_def);
    b2Polygon box         = b2MakeBox(4.0f, 4.0f);
    b2ShapeDef shape_def  = b2DefaultShapeDef();
    shape_def.density     = 1.0f;
    shape_def.friction    = 0.6f;
    shape_def.restitution = 0.1f;
    b2CreatePolygonShape(body, &shape_def, &box);
    command.spawn(body);
}

void toggle_full_screen(Query<Get<Window, ButtonInput<KeyCode>>> query) {
    if (!query.single().has_value()) return;
    auto [window, key_input] = query.single().value();
    if (key_input.just_pressed(pixel_engine::input::KeyF11)) {
        window.toggle_fullscreen();
    }
}

void render_bodies(
    Query<Get<b2BodyId>> query,
    Query<Get<pixel_engine::render::debug::vulkan::components::LineDrawer>>
        line_drawer_query,
    Query<Get<Device, Swapchain, Queue>, With<RenderContext>> context_query
) {
    if (!line_drawer_query.single().has_value()) return;
    if (!context_query.single().has_value()) return;
    auto [device, swap_chain, queue] = context_query.single().value();
    auto [line_drawer]               = line_drawer_query.single().value();
    line_drawer.begin(device, queue, swap_chain);
    for (auto [body] : query.iter()) {
        if (!b2Body_IsValid(body)) continue;
        b2Vec2 position  = b2Body_GetPosition(body);
        b2Vec2 velocity  = b2Body_GetLinearVelocity(body);
        auto mass_center = b2Body_GetWorldCenterOfMass(body);
        auto rotation    = b2Body_GetRotation(body);  // a cosine / sine pair
        auto angle       = std::atan2(rotation.s, rotation.c);
        glm::mat4 model  = glm::translate(
            glm::mat4(1.0f), {mass_center.x, mass_center.y, 0.0f}
        );
        line_drawer.setModel(model);
        line_drawer.drawLine(
            {0.0f, 0.0f, 0.0f}, {velocity.x / 2, velocity.y / 2, 0.0f},
            {0.0f, 0.0f, 1.0f, 1.0f}
        );
        model = glm::translate(glm::mat4(1.0f), {position.x, position.y, 0.0f});
        model = glm::rotate(model, angle, {0.0f, 0.0f, 1.0f});
        line_drawer.setModel(model);
        auto shape_count = b2Body_GetShapeCount(body);
        std::vector<b2ShapeId> shapes(shape_count);
        b2Body_GetShapes(body, shapes.data(), shape_count);
        for (auto&& shape : shapes) {
            if (b2Shape_GetType(shape) != b2ShapeType::b2_polygonShape)
                continue;
            auto polygon = b2Shape_GetPolygon(shape);
            for (size_t i = 0; i < polygon.count; i++) {
                auto& vertex1 = polygon.vertices[i];
                auto& vertex2 = polygon.vertices[(i + 1) % polygon.count];
                bool awake    = b2Body_IsAwake(body);
                line_drawer.drawLine(
                    {vertex1.x, vertex1.y, 0.0f}, {vertex2.x, vertex2.y, 0.0f},
                    {awake ? 0.0f : 1.0f, awake ? 1.0f : 0.0f, 0.0f, 0.3f}
                );
            }
        }
    }
    line_drawer.end();
}

void update_b2d_world(
    Query<Get<b2WorldId>> world_query, Local<std::optional<double>> last_time
) {
    if (!world_query.single().has_value()) return;
    auto [world] = world_query.single().value();
    if (!last_time->has_value()) {
        *last_time = std::chrono::duration<double>(
                         std::chrono::system_clock::now().time_since_epoch()
        )
                         .count();
        return;
    } else {
        auto current_time =
            std::chrono::duration<double>(
                std::chrono::system_clock::now().time_since_epoch()
            )
                .count();
        auto dt    = current_time - last_time->value();
        dt         = std::min(dt, 0.1);
        *last_time = current_time;
        b2World_Step(world, dt, 6);
    }
}

void destroy_too_far_bodies(
    Query<Get<Entity, b2BodyId>> query,
    Query<Get<b2WorldId>> world_query,
    Command command
) {
    if (!world_query.single().has_value()) return;
    auto [world] = world_query.single().value();
    for (auto [entity, body] : query.iter()) {
        auto position = b2Body_GetPosition(body);
        if (position.y < -1000.0f || position.y > 1000.0f ||
            position.x < -2000.0f || position.x > 2000.0f) {
            spdlog::info("Destroying body at {}, {}", position.x, position.y);
            b2DestroyBody(body);
            command.entity(entity).despawn();
        }
    }
}

void destroy_b2d_world(Query<Get<b2WorldId>> query) {
    for (auto [world] : query.iter()) {
        b2DestroyWorld(world);
    }
}

void create_text(
    Command command,
    ResMut<pixel_engine::font::resources::vulkan::FT2Library> ft2_library
) {
    if (!ft2_library.has_value()) return;
    pixel_engine::font::components::Text text;
    auto font_face =
        ft2_library->load_font("../assets/fonts/FiraSans-Bold.ttf");
    text.font = pixel_engine::font::components::Font{.font_face = font_face};
    text.font.pixels = 32;
    text.text =
        L"Hello, "
        L"Worldajthgreawiohguhiuwearjhoiughjoaewrughiowahioulgerjioweahjgiu"
        L"awhi"
        L"ohiouaewhoiughjwaoigoiehafgioerhiiUWEGHNVIOAHJEDSKGBHJIUAERWHJIUG"
        L"OHoa"
        L"ghweiuo ioweafgioewajiojoiatg huihkljh";
    command.spawn(text, pixel_engine::font::components::TextPos{100, 500});
    pixel_engine::font::components::Text text2;
    font_face = ft2_library->load_font(
        "../assets/fonts/HachicroUndertaleBattleFontRegular-L3zlg.ttf"
    );
    text2.font = pixel_engine::font::components::Font{.font_face = font_face};
    text2.font.pixels = 32;
    text2.text =
        L"Hello, "
        L"Worldoawgheruiahjgijanglkjwaehjgo;"
        L"ierwhjgiohnweaioulgfhjewjfweg3ioioiwefiowejhoiewfgjoiweaghioweahi"
        L"oawe"
        L"gHJWEAIOUHAWEFGIOULHJEAWio;hWE$gowaejgio";
    command.spawn(text2, pixel_engine::font::components::TextPos{100, 200});
}

void shuffle_text(Query<Get<pixel_engine::font::components::Text>> text_query) {
    for (auto [text] : text_query.iter()) {
        std::random_device rd;
        std::mt19937 g(rd());
        for (size_t i = 0; i < text.text.size(); i++) {
            std::uniform_int_distribution<> dis(0, text.text.size() - 1);
            text.text[i] = wchar_t(dis(g));
        }
    }
}

void create_pixel_block(Command command) {
    auto block =
        pixel_engine::render::pixel::components::PixelBlock::create({512, 512});
    for (size_t i = 0; i < block.size.x; i++) {
        for (size_t j = 0; j < block.size.y; j++) {
            block[{i, j}] = {
                std::rand() / (float)RAND_MAX, std::rand() / (float)RAND_MAX,
                std::rand() / (float)RAND_MAX, 1.0f
            };
        }
    }
    command.spawn(
        std::move(block),
        pixel_engine::render::pixel::components::BlockPos2d{
            .pos = {-1000, -1000, 0}, .scale = {4.0f, 4.0f}
        }
    );
}

void draw_lines(
    Query<Get<pixel_engine::render::debug::vulkan::components::LineDrawer>>
        query,
    Query<Get<Device, Swapchain, Queue>, With<RenderContext>> context_query
) {
    if (!query.single().has_value()) return;
    if (!context_query.single().has_value()) return;
    auto [device, swap_chain, queue] = context_query.single().value();
    auto [line_drawer]               = query.single().value();
    pixelbin bin(5, 5);
    bin.set(1, 1, true);
    bin.set(2, 1, true);
    bin.set(3, 1, true);
    bin.set(3, 2, true);
    // bin.set(3, 3, true);
    bin.set(2, 3, true);
    bin.set(1, 3, true);
    bin.set(1, 2, true);
    // for (int i = bin.size().x - 1; i >= 0; i--) {
    //     for (int j = 0; j < bin.size().y; j++) {
    //         std::cout << (bin.contains(i, j) ? "#" : "-");
    //     }
    //     std::cout << std::endl;
    // }
    auto outline    = find_outline(bin);
    auto simplified = douglas_peucker(outline, 0.1f);
    glm::mat4 model = glm::mat4(1.0f);
    model           = glm::translate(model, {0.0f, 0.0f, 0.0f});
    model           = glm::scale(model, {10.0f, 10.0f, 1.0f});
    line_drawer.begin(device, queue, swap_chain);
    line_drawer.setModel(model);
    // raw, green
    for (size_t i = 0; i < outline.size(); i++) {
        auto start = outline[i];
        auto end   = outline[(i + 1) % outline.size()];
        line_drawer.drawLine(
            {start.x, start.y, 0.0f}, {end.x, end.y, 0.0f},
            {0.0f, 1.0f, 0.0f, 1.0f}
        );
    }
    // simplified, red
    for (size_t i = 0; i < simplified.size(); i++) {
        auto start = simplified[i];
        auto end   = simplified[(i + 1) % simplified.size()];
        line_drawer.drawLine(
            {start.x, start.y, 0.0f}, {end.x, end.y, 0.0f},
            {1.0f, 0.0f, 0.0f, 1.0f}
        );
    }
    line_drawer.end();
}

void imgui_demo_window(ResMut<pixel_engine::imgui::ImGuiContext> imgui_context
) {
    if (!imgui_context.has_value()) return;
    ImGui::ShowDemoWindow();
}

void render_pixel_renderer_test(
    Query<Get<pixel_engine::render::pixel::components::PixelRenderer>> query,
    Query<Get<Device, Swapchain, Queue>, With<RenderContext>> context_query
) {
    using namespace pixel_engine::render::pixel::components;
    if (!query.single().has_value()) return;
    if (!context_query.single().has_value()) return;
    auto [device, swap_chain, queue] = context_query.single().value();
    auto [renderer]                  = query.single().value();
    PixelUniformBuffer pixel_uniform;
    pixel_uniform.view = glm::mat4(1.0f);
    pixel_uniform.proj = glm::ortho(
        -static_cast<float>(swap_chain.extent.width) / 2.0f,
        static_cast<float>(swap_chain.extent.width) / 2.0f,
        static_cast<float>(swap_chain.extent.height) / 2.0f,
        -static_cast<float>(swap_chain.extent.height) / 2.0f, 1000.0f, -1000.0f
    );
    renderer.begin(device, swap_chain, queue, pixel_uniform);
    renderer.set_model(
        glm::vec2(0.0f, 0.0f), glm::vec2(4.0f), glm::radians(45.0f)
    );
    for (int x = -10; x < 10; x++) {
        for (int y = -10; y < 10; y++) {
            renderer.draw({1.0f, 0.0f, 0.0f, 1.0f}, {x, y});
        }
    }
    renderer.end();
}

enum class SimulateState { Running, Paused };

void toggle_simulation(
    ResMut<NextState<SimulateState>> next_state,
    Query<Get<ButtonInput<KeyCode>>, With<PrimaryWindow>> query
) {
    if (!query.single().has_value()) return;
    auto [key_input] = query.single().value();
    if (key_input.just_pressed(pixel_engine::input::KeyEscape)) {
        if (next_state.has_value()) {
            next_state->set_state(
                next_state->is_state(SimulateState::Running)
                    ? SimulateState::Paused
                    : SimulateState::Running
            );
        }
    }
}

struct MouseJoint {
    b2BodyId body   = b2_nullBodyId;
    b2JointId joint = b2_nullJointId;
};

bool overlap_callback(b2ShapeId shapeId, void* context) {
    auto&& [world, mouse_joint, cursor] =
        *static_cast<std::tuple<b2WorldId, MouseJoint, b2Vec2>*>(context);
    auto body = b2Shape_GetBody(shapeId);
    if (b2Body_GetType(body) != b2BodyType::b2_dynamicBody) return true;
    spdlog::info("Overlap with dynamic");
    b2MouseJointDef def = b2DefaultMouseJointDef();
    def.bodyIdA         = body;
    def.bodyIdB         = body;
    def.target          = cursor;
    def.maxForce        = 100000.0f * b2Body_GetMass(body);
    def.dampingRatio    = 0.7f;
    def.hertz           = 5.0f;
    mouse_joint.joint   = b2CreateMouseJoint(world, &def);
    mouse_joint.body    = body;
    return false;
}
void update_mouse_joint(
    Command command,
    Query<Get<b2WorldId>> world_query,
    Query<Get<Entity, MouseJoint>> mouse_joint_query,
    Query<Get<const Window, const ButtonInput<MouseButton>>> input_query
) {
    if (!world_query.single().has_value()) return;
    if (!input_query.single().has_value()) return;
    auto [world]               = world_query.single().value();
    auto [window, mouse_input] = input_query.single().value();
    if (mouse_joint_query.single().has_value()) {
        auto [entity, joint] = mouse_joint_query.single().value();
        b2Vec2 cursor        = b2Vec2(
            window.get_cursor().x - window.get_size().width / 2,
            window.get_size().height / 2 - window.get_cursor().y
        );
        b2MouseJoint_SetTarget(joint.joint, cursor);
        if (!mouse_input.pressed(pixel_engine::input::MouseButton2)) {
            spdlog::info("Destroy Mouse Joint");
            b2DestroyJoint(joint.joint);
            command.entity(entity).despawn();
        }
    } else if (mouse_input.just_pressed(pixel_engine::input::MouseButton2) &&
               !ImGui::GetIO().WantCaptureMouse) {
        if (mouse_joint_query.single().has_value()) return;
        b2AABB aabb   = b2AABB();
        b2Vec2 cursor = b2Vec2(
            window.get_cursor().x - window.get_size().width / 2,
            window.get_size().height / 2 - window.get_cursor().y
        );
        std::tuple<b2WorldId, MouseJoint, b2Vec2> context = {world, {}, cursor};
        spdlog::info("Cursor at {}, {}", cursor.x, cursor.y);
        aabb.lowerBound      = cursor - b2Vec2(0.1f, 0.1f);
        aabb.upperBound      = cursor + b2Vec2(0.1f, 0.1f);
        b2QueryFilter filter = b2DefaultQueryFilter();
        b2World_OverlapAABB(world, aabb, filter, overlap_callback, &context);
        if (b2Joint_IsValid(std::get<1>(context).joint) &&
            b2Body_IsValid(std::get<1>(context).body)) {
            spdlog::info("Create Mouse Joint");
            command.spawn(std::get<1>(context));
            b2MouseJoint_SetTarget(std::get<1>(context).joint, cursor);
        }
    }
}

struct VK_TrialPlugin : Plugin {
    void build(App& app) override {
        auto window_plugin = app.get_plugin<WindowPlugin>();
        // window_plugin->primary_window_vsync = false;

        using namespace pixel_engine;

        app.enable_loop();
        // app.add_system(Startup, create_text);
        // app.add_system(Startup, create_pixel_block);
        // app.add_system(Render, draw_lines);
        app.insert_state(SimulateState::Running);
        app.add_system(
               Startup, create_b2d_world, create_ground, create_dynamic,
               create_pixel_block_with_collision
        )
            .chain();
        app.add_system(PreUpdate, update_b2d_world)
            .in_state(SimulateState::Running);
        app.add_system(PreUpdate, toggle_full_screen).use_worker("single");
        app.add_system(
            Update, destroy_too_far_bodies, create_dynamic_from_click,
            toggle_simulation, update_mouse_joint
        );
        app.add_system(Render, render_pixel_block);
        app.add_system(
               PostRender, render_bodies
               /*, imgui_demo_window, render_pixel_renderer_test */
        )
            .before(pixel_engine::render_vk::systems::present_frame);
        app.add_system(Exit, destroy_b2d_world);
    }
};

void run() {
    App app = App::create();
    app.enable_loop();
    app.add_plugin(pixel_engine::window::WindowPlugin{});
    app.add_plugin(pixel_engine::input::InputPlugin{}.enable_output());
    app.add_plugin(pixel_engine::render_vk::RenderVKPlugin{});
    app.add_plugin(pixel_engine::render::debug::vulkan::DebugRenderPlugin{});
    // app.add_plugin(pixel_engine::font::FontPlugin{});
    app.add_plugin(vk_trial::VK_TrialPlugin{});
    app.add_plugin(pixel_engine::imgui::ImGuiPluginVK{});
    app.add_plugin(pixel_engine::render::pixel::PixelRenderPlugin{});
    // app.add_plugin(pixel_engine::sprite::SpritePluginVK{});
    app.run();
}
}  // namespace vk_trial