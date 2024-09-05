#include <pixel_engine/app/app.h>

#include <iostream>

using namespace pixel_engine::app;

void function(Command command) { std::cout << "Hello, World!" << std::endl; }

int main() {
    World app;
    app.run_system(function);
    return 0;
}