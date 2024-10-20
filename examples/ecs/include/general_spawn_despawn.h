﻿#include <pixel_engine/entity.h>

#include <format>
#include <iomanip>
#include <iostream>
#include <random>

namespace test_spawn_despawn {
using namespace pixel_engine;
using namespace prelude;

struct Health {
    float life;
    Health(Health&& other) = default;
    Health(float life = 100.0f) : life(life) {}
    Health& operator=(const Health& other) = delete;
    Health(const Health& other) = delete;
    Health& operator=(Health&& other) = default;
    ~Health() { std::cout << "Health destructor" << std::endl; }
};

struct Position {
    float x, y;
    Position(Position&& other) = default;
    Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
    Position& operator=(const Position& other) = delete;
    Position(const Position& other) = delete;
    Position& operator=(Position&& other) = default;
    ~Position() { std::cout << "Position destructor" << std::endl; }
};

struct InnerBundle : Bundle {
    int a;

    auto unpack() { return std::tie(a); }
};

struct HealthPositionBundle : Bundle {
    InnerBundle inner;
    Health health;
    Position position;

    auto unpack() {
        return std::forward_as_tuple(
            std::move(inner), std::move(health), std::move(position)
        );
    }
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
            command.spawn(Health{});
        }
    }
    std::cout << std::endl;
}

void print_1(Query<Get<Entity, Health, Position>, With<>, Without<>> query) {
    std::cout << "print" << std::endl;
    for (auto [entity, health, position] : query.iter()) {
        std::string id = std::format("{:#05x}", static_cast<int>(entity));
        std::cout << "entity: " << id << " [health: " << health.life
                  << " position: " << position.x << ", " << position.y << "]"
                  << " " << query.contains(entity) << std::endl;
    }
    std::cout << std::endl;
}

void print_2(Query<Get<Entity, Health, Position>, With<>, Without<>> query) {
    std::cout << "print" << std::endl;
    for (auto [entity, health, position] : query.iter()) {
        std::string id = std::format("{:#05x}", static_cast<int>(entity));
        std::cout << "entity: " << id << " [health: " << health.life
                  << " position: " << position.x << ", " << position.y << "]"
                  << " " << query.contains(entity) << std::endl;
    }
    std::cout << std::endl;
}

void print_count_1(Query<Get<Entity, Health>, With<>, Without<>> query) {
    std::cout << "print_count" << std::endl;
    int count = 0;
    for (auto [entity, health] : query.iter()) {
        count++;
    }
    std::cout << "entity count: " << count << std::endl;
    std::cout << std::endl;
}

void print_count_2(Query<Get<Entity, Health>, With<>, Without<>> query) {
    std::cout << "print_count" << std::endl;
    int count = 0;
    for (auto [entity, health] : query.iter()) {
        count++;
    }
    std::cout << "entity count: " << count << std::endl;
    std::cout << std::endl;
}

void print_count_3(Query<Get<Entity, Health>, With<>, Without<>> query) {
    std::cout << "print_count" << std::endl;
    int count = 0;
    for (auto [entity, health] : query.iter()) {
        count++;
    }
    std::cout << "entity count: " << count << std::endl;
    std::cout << std::endl;
}

void change_component_data(
    Command command, Query<Get<Entity, Health>, With<>, Without<>> query
) {
    std::cout << "change_component_data" << std::endl;
    for (auto [id, health] : query.iter()) {
        auto [heal2] = query.get(id);
        heal2.life = 200.0f;
    }
    std::cout << std::endl;
}

void despawn(
    Command command, Query<Get<Entity, Health>, With<>, Without<>> query
) {
    std::cout << "despawn" << std::endl;
    for (auto [entity, health] : query.iter()) {
        command.entity(entity).despawn();
    }
    std::cout << std::endl;
}

class SpawnDespawnPlugin : public Plugin {
   public:
    void build(App& app) override {
        app.add_system(spawn)
            .in_stage(app::Startup)
            ->add_system(print_count_1)
            .after(spawn)
            .in_stage(app::Startup)
            ->add_system(print_1)
            .after(print_count_1)
            .in_stage(app::Startup)
            ->add_system(change_component_data)
            .after(print_1)
            .in_stage(app::Startup)
            ->add_system(print_count_2)
            .after(change_component_data)
            .in_stage(app::Startup)
            ->add_system(print_2)
            .after(print_count_2)
            .in_stage(app::Startup)
            ->add_system(despawn)
            .after(print_2)
            .in_stage(app::Startup)
            ->add_system(print_count_3)
            .in_stage(app::Update);
    }
};

void test() {
    App app;
    app.add_plugin(SpawnDespawnPlugin{}).run();
}
}  // namespace test_spawn_despawn