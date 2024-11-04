#include <pixel_engine/app.h>

using namespace pixel_engine::prelude;

void func1() {
    std::cout << "func1" << std::endl;
}
void func2() {
    std::cout << "func2" << std::endl;
}

int main() {
    auto app = App::create();
    app.add_system(Startup, func1);
    app.add_system(Startup, func2).after(func1);
    app.run();
    return 0;
}