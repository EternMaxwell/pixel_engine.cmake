#include <pixel_engine/entity.h>

#include <format>
#include <iomanip>
#include <iostream>
#include <random>

namespace test_spawn_despawn {
    using namespace pixel_engine;
    using namespace prelude;

    struct Health {
        float life;
    };

    struct Position {
        float x, y;
    };

    struct HealthPositionBundle {
        Bundle tag;
        Health health{.life = 100.0f};
        Position position{.x = 0.0f, .y = 0.0f};
    };

    void spawn(Command command) {
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

    void print(Query<Get<Entity, Health, Position>, With<>, Without<>> query) {
        std::cout << "print" << std::endl;
        for (auto [entity, health, position] : query.iter()) {
            std::string id = std::format("{:#05x}", static_cast<int>(entity));
            std::cout << "entity: " << id << " [health: " << health.life << " position: " << position.x << ", "
                      << position.y << "]"
                      << " " << query.contains(entity) << std::endl;
        }
        std::cout << std::endl;
    }

    void print_count(Query<Get<Entity, Health>, With<>, Without<>> query) {
        std::cout << "print_count" << std::endl;
        int count = 0;
        for (auto [entity, health] : query.iter()) {
            count++;
        }
        std::cout << "entity count: " << count << std::endl;
        std::cout << std::endl;
    }

    void change_component_data(Command command, Query<Get<Entity, Health>, With<>, Without<>> query) {
        std::cout << "change_component_data" << std::endl;
        for (auto [id, health] : query.iter()) {
            auto [heal2] = query.get(id);
            heal2.life = 200.0f;
        }
        std::cout << std::endl;
    }

    void despawn(Command command, Query<Get<Entity, Health>, With<>, Without<>> query) {
        std::cout << "despawn" << std::endl;
        for (auto [entity, health] : query.iter()) {
            command.entity(entity).despawn();
        }
        std::cout << std::endl;
    }

    class SpawnDespawnPlugin : public Plugin {
       public:
        void build(App& app) override {
            SystemNode node;
            app.add_system(Startup{}, spawn, &node)
                .add_system(Startup{}, print_count, &node, after(node))
                .add_system(Startup{}, print, &node, after(node))
                .add_system(Startup{}, change_component_data, &node, after(node))
                .add_system(Startup{}, print_count, &node, after(node))
                .add_system(Startup{}, print, &node, after(node))
                .add_system(Startup{}, despawn, &node, after(node))
                .add_system(Update{}, print_count, &node, after(node));
        }
    };

    void test() {
        App app;
        app.add_plugin(SpawnDespawnPlugin{}).run();
    }
}  // namespace test_spawn_despawn