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
            std::cout << "entity with child: "
                      << std::format("{:#05x}", static_cast<int>(entity))
                      << std::endl;
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

    void despawn_recurese(
        entity::Command command,
        entity::Query<std::tuple<entt::entity, WithChild>, std::tuple<>> query) {
        std::cout << "despawn" << std::endl;
        for (auto [entity] : query.iter()) {
            command.entity(entity).despawn_recurse();
        }
        std::cout << std::endl;
    }

    void test() {
        entity::App app;
        app.add_system(spawn)
            .add_system(print)
            .add_system(create_child)
            .add_system(print)
            .add_system(despawn_recurese)
            .add_system(print)
            .run();
    }
}  // namespace test_with_child