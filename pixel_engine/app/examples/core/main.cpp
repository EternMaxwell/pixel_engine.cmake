#include <pixel_engine/app/app.h>

#include <iostream>

using namespace pixel_engine::app;

enum Sets { a, b };

void function(Command command) { std::cout << "Hello, World!" << std::endl; }

void function2(Command command) { std::cout << "Hello, World2!" << std::endl; }

int main() {
    spdlog::set_level(spdlog::level::debug);
    App app;
    app.log_level(spdlog::level::debug).configure_sets(a, b)
        .add_system(Schedule(Startup), function, in_set(b))
        .use_worker("single")
        ->add_system(Schedule(Startup), function2, in_set(a))
        ->run();
    return 0;
}