#include <pixel_engine/entity/entity.h>

#include <iomanip>
#include <iostream>
#include <random>
#include <format>

namespace test_spawn_despawn{
    using namespace pixel_engine;

    struct Health {
        float life;
    };

    struct Position {
        float x, y;
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
                command.spawn(Health{.life = 100.0f}, Position{.x = 0, .y = 0});
            } else {
                command.spawn(Health{.life = 100.0f});
            }
        }
        std::cout << std::endl;
    }

    void print(
        entity::Query<std::tuple<entt::entity, Health, Position>, std::tuple<>>
            query) {
        std::cout << "print" << std::endl;
        for (auto [entity, health, position] : query.iter()) {
            std::string id = std::format("{:#05x}", static_cast<int>(entity));
            std::cout << "entity: " << id
                      << " [health: " << health.life
                      << " position: " << position.x << ", " << position.y
                      << "]" << std::endl;
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

    void change_component_data(
		entity::Command command,
		entity::Query<std::tuple<entt::entity, Health>, std::tuple<>> query) {
		std::cout << "change_component_data" << std::endl;
		for (auto [entity, health] : query.iter()) {
			health.life = 200.0f;
		}
		std::cout << std::endl;
	}

    void despawn(
        entity::Command command,
        entity::Query<std::tuple<entt::entity, Health>, std::tuple<>> query) {
        std::cout << "despawn" << std::endl;
        for (auto [entity, health] : query.iter()) {
            command.entity(entity).despawn();
        }
        std::cout << std::endl;
    }

    void test() {
        entity::App app;
        app.add_system(spawn)
            .add_system(print_count)
            .add_system(change_component_data)
            .add_system(print_count)
            .add_system(despawn)
            .add_system(print_count)
            .run();
    }
}