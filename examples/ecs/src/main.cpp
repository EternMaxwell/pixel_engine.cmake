#include <iostream>
#include <random>

#include "ecs/registry_wrapper.h"

struct Health {
    float life;
};

struct Position {
    float x, y;
};

void start(ecs_trial::Command command) {
    for (int i = 0; i < 500; i++) {
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
}

void print(
    ecs_trial::Command command,
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

    return 0;
}