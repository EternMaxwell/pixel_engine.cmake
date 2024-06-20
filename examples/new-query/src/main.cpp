#include <pixel_engine/prelude.h>

using namespace pixel_engine;
using namespace prelude;
#include <format>
#include <iomanip>
#include <iostream>
#include <random>

struct Health {
    float life;
};

struct Position {
    float x, y;
};

struct Tag {};

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
            command.spawn(Health{.life = 100.0f}, Position{0, 0}, Tag{});
        } else {
            command.spawn(Health{.life = 100.0f}, Tag{});
        }
    }
    std::cout << std::endl;
}

void print(entity::query::Query2<Get<entt::entity, Health, Position>, With<Tag>, Without<>> query) {
    std::cout << "print" << std::endl;
    for (auto [entity, health, position] : query.iter()) {
        std::string id = std::format("{:#05x}", static_cast<int>(entity));
        std::cout << "entity: " << id << " [health: " << health.life << " position: " << position.x << ", "
                  << position.y << "]" << std::endl;
    }
    std::cout << std::endl;
}

void change(entity::query::Query2<Get<Health, Position>, With<Tag>, Without<>> query) {
    std::cout << "change" << std::endl;
    for (auto [health, position] : query.iter()) {
        health.life -= 1.0f;
        position.x += 1.0f;
        position.y += 1.0f;
    }
    std::cout << std::endl;
}

int main() {
    App app;
    app.add_system(Startup{}, spawn)
        .add_system(Update{}, print)
        .add_system(Update{}, change)
        .add_system(Update{}, print)
        .run();
}