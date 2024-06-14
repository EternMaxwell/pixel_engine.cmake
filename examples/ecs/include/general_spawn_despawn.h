#include <pixel_engine/entity.h>

#include <format>
#include <iomanip>
#include <iostream>
#include <random>

namespace test_spawn_despawn {
    using namespace pixel_engine;
    using namespace entity;

    struct Health {
        float life;
    };

    struct Position {
        float x, y;
    };

    struct HealthPositionBundle {
        entity::Bundle tag;
        Health health{.life = 100.0f};
        Position position{.x = 0.0f, .y = 0.0f};
    };

    void spawn(entity::Command command) {
        std::cout << "spawn" << std::endl;
        for (int i = 0; i < 50; i++) {
            using namespace std;
            int min = 0, max = 100;
            random_device seed;
            ranlux48 engine(seed());
            uniform_int_distribution<> distrib(min, max);
            int random = distrib(engine);
            if (random > 50) {
                command.spawn(HealthPositionBundle());
            } else {
                command.spawn(Health{.life = 100.0f});
            }
        }
        std::cout << std::endl;
    }

    void print(entity::Query<Get<entt::entity, Health, Position>, Without<>> query) {
        std::cout << "print" << std::endl;
        for (auto [entity, health, position] : query.iter()) {
            std::string id = std::format("{:#05x}", static_cast<int>(entity));
            std::cout << "entity: " << id << " [health: " << health.life << " position: " << position.x << ", "
                      << position.y << "]" << std::endl;
        }
        std::cout << std::endl;
    }

    void print_count(entity::Query<Get<entt::entity, Health>, Without<>> query) {
        std::cout << "print_count" << std::endl;
        int count = 0;
        for (auto [entity, health] : query.iter()) {
            count++;
        }
        std::cout << "entity count: " << count << std::endl;
        std::cout << std::endl;
    }

    void change_component_data(entity::Command command,
                               entity::Query<Get<Health>, Without<>> query) {
        std::cout << "change_component_data" << std::endl;
        for (auto [health] : query.iter()) {
            health.life = 200.0f;
        }
        std::cout << std::endl;
    }

    void despawn(entity::Command command, entity::Query<Get<entt::entity, Health>, Without<>> query) {
        std::cout << "despawn" << std::endl;
        for (auto [entity, health] : query.iter()) {
            command.entity(entity).despawn();
        }
        std::cout << std::endl;
    }

    class SpawnDespawnPlugin : public entity::Plugin {
       public:
        void build(entity::App& app) override {
            app.add_system(entity::Startup{}, spawn)
                .add_system(entity::Startup{}, print_count)
                .add_system(entity::Startup{}, print)
                .add_system(entity::Startup{}, change_component_data)
                .add_system(entity::Startup{}, print_count)
                .add_system(entity::Startup{}, print)
                .add_system(entity::Startup{}, despawn)
                .add_system(entity::Update{}, print_count);
        }
    };

    void test() {
        entity::App app;
        app.add_plugin(SpawnDespawnPlugin{}).run();
    }
}  // namespace test_spawn_despawn