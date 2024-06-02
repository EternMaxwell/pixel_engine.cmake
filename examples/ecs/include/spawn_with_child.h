#include <pixel_engine/entity/entity.h>

#include <format>
#include <iomanip>
#include <iostream>
#include <random>

namespace test_with_child {
    using namespace pixel_engine;

    struct Health {
        float life;
    };

    struct Position {
        float x, y;
    };

    struct WithChild {};

    void spawn(entity::Command command) {
        std::cout << "spawn" << std::endl;
        for (int i = 0; i < 50; i++) {
            command.spawn(Health{.life = 100.0f}, WithChild{});
        }
        std::cout << std::endl;
    }

    void create_child(
        entity::Command command,
        entity::Query<std::tuple<entt::entity, WithChild>, std::tuple<>>
            query) {
        std::cout << "create_child" << std::endl;
        for (auto [entity] : query.iter()) {
            command.entity(entity).spawn(Health{.life = 50.0f});
        }
        std::cout << std::endl;
    }

    void print(
        entity::Query<std::tuple<entt::entity, Health>, std::tuple<>> query) {
        std::cout << "print" << std::endl;
        for (auto [entity, health] : query.iter()) {
            std::string id = std::format("{:#05x}", static_cast<int>(entity));
            std::cout << "entity: " << id << " [health: " << health.life << "]"
                      << std::endl;
        }
        std::cout << std::endl;
    }

    void print_count(
        entity::Query<std::tuple<entt::entity, Health>, std::tuple<>> query) {
        std::cout << "print_count" << std::endl;
        int count = 0;
        for (auto [entity, health] : query.iter()) {
            count++;
        }
        std::cout << "entity count: " << count << std::endl;
        std::cout << std::endl;
    }

    void despawn_recurese(
        entity::Command command,
        entity::Query<std::tuple<entt::entity, WithChild>, std::tuple<>>
            query) {
        std::cout << "despawn" << std::endl;
        for (auto [entity] : query.iter()) {
            command.entity(entity).despawn_recurse();
        }
        std::cout << std::endl;
    }

    void test() {
        entity::App app;
        app.add_system(entity::Startup{}, spawn)
            .add_system(entity::Startup{}, print_count)
            .add_system(entity::Startup{}, create_child)
            .add_system(entity::Startup{}, print_count)
            .add_system(entity::Startup{}, despawn_recurese)
            .add_system(entity::Startup{}, print_count)
            .run();
    }
}  // namespace test_with_child