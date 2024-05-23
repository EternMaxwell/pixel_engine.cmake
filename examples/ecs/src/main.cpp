#include <iostream>
#include <random>

#include "ecs/registry_wrapper.h"

struct Health {
    float life;
};

struct Position {
    float x, y;
};

struct WithChild {};

void start(ecs_trial::Command command) {
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
            command.spawn(Health{.life = 100.0f}, WithChild{});
        }
    }
}

void create_child(ecs_trial::Command command,
                  ecs_trial::Query<std::tuple<WithChild>, std::tuple<>> query) {
    for (auto [entity] : query.iter()) {
        std::cout << "entity with child: " << static_cast<int>(entity)
                  << std::endl;
        command.with_child(entity, Health{.life = 50},
                           Position{.x = 0, .y = 0});
    }
}

void despawn(ecs_trial::Command command,
             ecs_trial::Query<std::tuple<WithChild>, std::tuple<>> query) {
    for (auto [entity] : query.iter()) {
        command.despawn_recurse(entity);
    }
}

void print(
    ecs_trial::Query<std::tuple<const Health, const Position>, std::tuple<>>
        query) {
    for (auto [entity, health, pos] : query.iter()) {
        std::cout << static_cast<int>(entity) << ": " << health.life
                  << std::endl;
    }
}

int main() {
    ecs_trial::App app;
    app.run_system(start).run_system(print);
    app.run_system(create_child).run_system(print);
    app.run_system(despawn).run_system(print);

    return 0;
}