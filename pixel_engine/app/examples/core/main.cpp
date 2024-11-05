#include <pixel_engine/app.h>

#include <iostream>

using namespace pixel_engine::prelude;
using namespace pixel_engine;

enum Sets { a, b };

void function(Command command) { std::cout << "Hello, World!" << std::endl; }

void function2(Command command) { std::cout << "Hello, World2!" << std::endl; }

int main() {
    spdlog::set_level(spdlog::level::debug);
    App app;
    app->log_level(spdlog::level::debug)
        ->configure_sets(a, b)
        ->add_system(function)
        .in_set(b)
        .use_worker("single")
        .in_stage(app::Startup)
        ->add_system(function2)
        .in_set(a)
        .in_stage(app::Startup)
        ->run();
    return 0;
}