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
#include <stb_image.h>

#include <random>

#include "fragment_shader.h"
#include "vertex_shader.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine::window;
using namespace pixel_engine::render_vk::components;
using namespace pixel_engine::sprite::components;
using namespace pixel_engine::sprite::resources;

namespace vk_trial {
void create_b2d_world(Command command) {
    b2WorldDef def = b2DefaultWorldDef();
    def.gravity    = {0.0f, -29.8f};
    auto world     = b2CreateWorld(&def);
    command.spawn(world);
}

void create_ground(Command command, Query<Get<b2WorldId>> world_query) {
    if (!world_query.single().has_value()) return;
    auto [world]         = world_query.single().value();
    b2BodyDef body_def   = b2DefaultBodyDef();
    body_def.type        = b2BodyType::b2_staticBody;
    body_def.position    = {0.0f, -100.0f};
    auto ground          = b2CreateBody(world, &body_def);
    b2Polygon ground_box = b2MakeBox(500.0f, 10.0f);
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
    b2Polygon box         = b2MakeBox(10.0f, 10.0f);
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
    if (!mouse_input.just_pressed(pixel_engine::input::MouseButton1) &&
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
    b2Polygon box         = b2MakeBox(10.0f, 10.0f);
    b2ShapeDef shape_def  = b2DefaultShapeDef();
    shape_def.density     = 1.0f;
    shape_def.friction    = 0.6f;
    shape_def.restitution = 0.1f;
    b2CreatePolygonShape(body, &shape_def, &box);
    command.spawn(body);
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
        b2Vec2 position = b2Body_GetPosition(body);
        b2Vec2 velocity = b2Body_GetLinearVelocity(body);
        auto rotation   = b2Body_GetRotation(body);  // a cosine / sine pair
        auto angle      = std::atan2(rotation.s, rotation.c);
        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), {position.x, position.y, 0.0f});
        line_drawer.setModel(model);
        line_drawer.drawLine(
            {0.0f, 0.0f, 0.0f}, {velocity.x / 2, velocity.y / 2, 0.0f},
            {0.0f, 0.0f, 1.0f, 1.0f}
        );
        model = glm::rotate(model, angle, {0.0f, 0.0f, 1.0f});
        line_drawer.setModel(model);
        auto shape_count = b2Body_GetShapeCount(body);
        std::vector<b2ShapeId> shapes(shape_count);
        b2Body_GetShapes(body, shapes.data(), shape_count);
        for (auto shape : shapes) {
            if (b2Shape_GetType(shape) != b2ShapeType::b2_polygonShape)
                continue;
            auto polygon = b2Shape_GetPolygon(shape);
            auto center  = polygon.centroid;
            for (size_t i = 0; i < polygon.count; i++) {
                auto& vertex1 = polygon.vertices[i];
                auto& vertex2 = polygon.vertices[(i + 1) % polygon.count];
                bool awake    = b2Body_IsAwake(body);
                line_drawer.drawLine(
                    {vertex1.x + center.x, vertex1.y + center.y, 0.0f},
                    {vertex2.x + center.x, vertex2.y + center.y, 0.0f},
                    {awake ? 0.0f : 1.0f, awake ? 1.0f : 0.0f, 0.0f, 1.0f}
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
        L"Worldajthgreawiohguhiuwearjhoiughjoaewrughiowahioulgerjioweahjgiuawhi"
        L"ohiouaewhoiughjwaoigoiehafgioerhiiUWEGHNVIOAHJEDSKGBHJIUAERWHJIUGOHoa"
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
        L"ierwhjgiohnweaioulgfhjewjfweg3ioioiwefiowejhoiewfgjoiweaghioweahioawe"
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
    line_drawer.begin(device, queue, swap_chain);
    line_drawer.drawLine({0, 0, 0}, {100, 100, 0}, {1.0f, 0.0f, 0.0f, 1.0f});
    line_drawer.end();
}

void imgui_demo_window(ResMut<pixel_engine::imgui::ImGuiContext> imgui_context
) {
    if (!imgui_context.has_value()) return;
    ImGui::ShowDemoWindow();
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
        app.add_system(Render, imgui_demo_window);

        app.add_system(Startup, create_b2d_world);
        app.add_system(Startup, create_ground).after(create_b2d_world);
        app.add_system(Startup, create_dynamic)
            .after(create_b2d_world, create_ground);
        app.add_system(PreUpdate, update_b2d_world);
        app.add_system(Update, destroy_too_far_bodies);
        app.add_system(Update, create_dynamic_from_click);
        app.add_system(Render, render_bodies);
        app.add_system(Exit, destroy_b2d_world);
    }
};

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
    bool at(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) return false;
        int index = x / 32 + y * column;
        int bit   = x % 32;
        return pixels[index] & (1 << bit);
    }
    glm::ivec2 size() const { return {width, height}; }
};

std::vector<glm::ivec2> find_outline(const pixelbin& pixelbin) {
    std::vector<glm::ivec2> outline;
    std::vector<glm::ivec2> move    = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};
    std::vector<glm::ivec2> offsets = {
        {-1, -1}, {-1, 0}, {0, 0}, {0, -1}
    };  // if dir is i then in ccw order, i+1 is the right pixel coord, i is
        // the left pixel coord, i = 0 means left.
    glm::ivec2 start(-1, -1);
    for (int i = 0; i < pixelbin.size().x; i++) {
        for (int j = 0; j < pixelbin.size().y; j++) {
            if (pixelbin.at(i, j)) {
                start = {i, j};
                break;
            }
        }
        if (start.x != -1) break;
    }
    if (start.x == -1) return outline;
    glm::ivec2 current = start;
    int dir            = 0;
    do {
        outline.push_back(current);
        for (int ndir = (dir + 3) % 4; ndir != (dir + 2) % 4;
             ndir     = (ndir + 1) % 4) {
            auto outside = current + offsets[ndir];
            auto inside  = current + offsets[(ndir + 1) % 4];
            if (!pixelbin[outside] && pixelbin[inside]) {
                current = current + move[ndir];
                dir     = ndir;
                break;
            }
        }
    } while (current != start);
    return outline;
}

void run() {
    App app = App::create();
    app.enable_loop();
    app.add_plugin(pixel_engine::window::WindowPlugin{});
    app.add_plugin(pixel_engine::input::InputPlugin{});
    app.add_plugin(pixel_engine::render_vk::RenderVKPlugin{});
    app.add_plugin(pixel_engine::render::debug::vulkan::DebugRenderPlugin{});
    // app.add_plugin(pixel_engine::font::FontPlugin{});
    app.add_plugin(vk_trial::VK_TrialPlugin{});
    app.add_plugin(pixel_engine::imgui::ImGuiPluginVK{});
    // app.add_plugin(pixel_engine::render::pixel::PixelRenderPlugin{});
    // app.add_plugin(pixel_engine::sprite::SpritePluginVK{});
    // app.run();

    pixelbin bin(5, 5);
    bin.set(1, 1, true);
    bin.set(2, 1, true);
    bin.set(3, 1, true);
    bin.set(3, 2, true);
    // bin.set(3, 3, true);
    bin.set(2, 3, true);
    bin.set(1, 3, true);
    bin.set(1, 2, true);
    for (int i = bin.size().x - 1; i >= 0; i--) {
        for (int j = 0; j < bin.size().y; j++) {
            std::cout << (bin.at(i, j) ? "#" : "-");
        }
        std::cout << std::endl;
    }
    auto outline = find_outline(bin);
    for (auto pos : outline) {
        std::cout << std::format("({}, {}) ", pos.x, pos.y);
    }
}
}  // namespace vk_trial