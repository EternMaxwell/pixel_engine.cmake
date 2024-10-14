#include <pixel_engine/entity.h>

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

void create_child(
    Command command, Query<Get<Entity>, With<WithChild>, Without<>> query
) {
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
        std::cout << "entity: " << id << " [health: " << health.life << "]"
                  << std::endl;
    }
    std::cout << std::endl;
}

void print_count1(Query<Get<Health>, With<>, Without<>> query) {
    std::cout << "print_count" << std::endl;
    int count = 0;
    for (auto [health] : query.iter()) {
        count++;
    }
    std::cout << "entity count: " << count << std::endl;
    std::cout << std::endl;
}

void print_count2(Query<Get<Health>, With<>, Without<>> query) {
    std::cout << "print_count" << std::endl;
    int count = 0;
    for (auto [health] : query.iter()) {
        count++;
    }
    std::cout << "entity count: " << count << std::endl;
    std::cout << std::endl;
}

void print_count3(Query<Get<Health>, With<>, Without<>> query) {
    std::cout << "print_count" << std::endl;
    int count = 0;
    for (auto [health] : query.iter()) {
        count++;
    }
    std::cout << "entity count: " << count << std::endl;
    std::cout << std::endl;
}

void despawn_recurese(
    Command command, Query<Get<Entity>, With<WithChild>, Without<>> query
) {
    std::cout << "despawn" << std::endl;
    for (auto [entity] : query.iter()) {
        command.entity(entity).despawn_recurse();
    }
    std::cout << std::endl;
}

class SpawnWithChildPlugin : public Plugin {
   public:
    void build(App& app) override {
        app.add_system(spawn)
            .in_stage(app::Startup)
            ->add_system(print_count1)
            .in_stage(app::Startup)
            .after(spawn)
            ->add_system(create_child)
            .in_stage(app::Startup)
            .after(print_count1)
            ->add_system(print_count2)
            .in_stage(app::Startup)
            .after(create_child)
            ->add_system(despawn_recurese)
            .in_stage(app::Startup)
            .after(print_count2)
            ->add_system(print_count3)
            .in_stage(app::Update);
    }
};

void test() {
    App app;
    app.add_plugin(SpawnWithChildPlugin{}).run();
}
}  // namespace test_with_child