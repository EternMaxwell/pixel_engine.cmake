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
    std::cout << "start" << std::endl;
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
    std::cout << std::endl;
}

void create_child(ecs_trial::Command command,
                  ecs_trial::Query<std::tuple<WithChild>, std::tuple<>> query) {
    std::cout << "create_child" << std::endl;
    for (auto [entity] : query.iter()) {
        std::cout << "entity with child: " << static_cast<int>(entity)
                  << std::endl;
        command.entity(entity).spawn(Health{.life = 50},
                           Position{.x = 0, .y = 0});
    }
    std::cout << std::endl;
}

void despawn(ecs_trial::Command command,
             ecs_trial::Query<std::tuple<WithChild>, std::tuple<>> query) {
    std::cout << "despawn" << std::endl;
    for (auto [entity] : query.iter()) {
        command.entity(entity).despawn_recurse();
    }
    std::cout << std::endl;
}

void add_res(ecs_trial::Command command) {
    std::cout << "add_res" << std::endl;
    command.insert_resource(Health{.life = 100.0f});
    std::cout << std::endl;
}

void print(
    ecs_trial::Query<std::tuple<const Health, const Position>, std::tuple<>>
        query) {
    std::cout << "print" << std::endl;
    auto [entity, health, pos] = query.single().value();
    std::cout << "single entity: " << static_cast<int>(entity) << ": "
              << health.life << std::endl;

    for (auto [entity, health, pos] : query.iter()) {
        std::cout << static_cast<int>(entity) << ": " << health.life
                  << std::endl;
    }
    std::cout << std::endl;
}

void print_res(ecs_trial::Resource<const Health> res) {
    std::cout << "print_res" << std::endl;
    if (res.has_value()) {
        std::cout << "resource: " << res.value().life << std::endl;
    }
    std::cout << std::endl;
}

void access_not_exist_res(ecs_trial::Resource<Position> res) {
    std::cout << "access_not_exist_res" << std::endl;
    if (res.has_value()) {
        std::cout << "resource: " << res.value().x << std::endl;
    } else {
        std::cout << "resource not exist" << std::endl;
    }
    std::cout << std::endl;
}

void add_not_exist_res(ecs_trial::Command command) {
	std::cout << "add_not_exist_res" << std::endl;
	command.init_resource<Position>();
	std::cout << std::endl;
}

void remove_not_exist_res(ecs_trial::Command command) {
	std::cout << "remove_not_exist_res" << std::endl;
	command.remove_resource<Position>();
	std::cout << std::endl;
}

void change_res(ecs_trial::Resource<Health> res) {
    std::cout << "change_res" << std::endl;
    if (res.has_value()) {
        res.value().life = 50.0f;
    }
    std::cout << std::endl;
}

void write_event(ecs_trial::EventWriter<Health> writer) {
    std::cout << "write_event" << std::endl;
	writer.write(Health{.life = 100.0f});
    writer.write(Health{.life = 50.0f});
    writer.write(Health{.life = 10.0f});
    std::cout << std::endl;
}

void read_event(ecs_trial::EventReader<Health> reader) {
	std::cout << "read_event" << std::endl;
	for (auto health : reader.read()) {
		std::cout << "event: " << health.life << std::endl;
	}
	std::cout << std::endl;
}

int main() {
    ecs_trial::App app;
    app.run_system(start).run_system(print);
    app.run_system(create_child).run_system(print);
    app.run_system(despawn).run_system(print);
    app.run_system(add_res).run_system(print_res);
    app.run_system(change_res).run_system(print_res);
    app.run_system(access_not_exist_res);
    app.run_system(add_not_exist_res).run_system(access_not_exist_res);
    app.run_system(remove_not_exist_res).run_system(access_not_exist_res);
    app.run_system(write_event).run_system(read_event);

    return 0;
}