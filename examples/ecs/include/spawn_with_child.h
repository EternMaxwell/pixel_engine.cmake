﻿#include <pixel_engine/entity.h>

#include <format>
#include <iomanip>
#include <iostream>
#include <random>

namespace test_with_child {
    using namespace pixel_engine;
    using namespace prelude;

    struct Health {
        float life;
    };

    struct Position {
        float x, y;
    };

    struct WithChild {};

    void spawn(Command command) {
        std::cout << "spawn" << std::endl;
        for (int i = 0; i < 50; i++) {
            command.spawn(Health{.life = 100.0f}, WithChild{});
        }
        std::cout << std::endl;
    }

    void create_child(Command command, Query<Get<Entity>, With<WithChild>, Without<>> query) {
        std::cout << "create_child" << std::endl;
        for (auto [entity] : query.iter()) {
            command.entity(entity).spawn(Health{.life = 50.0f});
        }
        std::cout << std::endl;
    }

    void print(Query<Get<Entity, Health>, With<>, Without<>> query) {
        std::cout << "print" << std::endl;
        for (auto [entity, health] : query.iter()) {
            std::string id = std::format("{:#05x}", static_cast<int>(entity));
            std::cout << "entity: " << id << " [health: " << health.life << "]" << std::endl;
        }
        std::cout << std::endl;
    }

    void print_count(Query<Get<Health>, With<>, Without<>> query) {
        std::cout << "print_count" << std::endl;
        int count = 0;
        for (auto [health] : query.iter()) {
            count++;
        }
        std::cout << "entity count: " << count << std::endl;
        std::cout << std::endl;
    }

    void despawn_recurese(Command command, Query<Get<Entity>, With<WithChild>, Without<>> query) {
        std::cout << "despawn" << std::endl;
        for (auto [entity] : query.iter()) {
            command.entity(entity).despawn_recurse();
        }
        std::cout << std::endl;
    }

    class SpawnWithChildPlugin : public entity::Plugin {
       public:
        void build(entity::App& app) override {
            SystemNode node;
            app.add_system(Startup{}, spawn, &node)
                .add_system(Startup{}, print_count, &node, after(node))
                .add_system(Startup{}, create_child, &node, after(node))
                .add_system(Startup{}, print_count, &node, after(node))
                .add_system(Startup{}, despawn_recurese, &node, after(node))
                .add_system(Update{}, print_count);
        }
    };

    void test() {
        entity::App app;
        app.add_plugin(SpawnWithChildPlugin{}).run();
    }
}  // namespace test_with_child