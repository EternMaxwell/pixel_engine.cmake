#include <pixel_engine/entity.h>

namespace test_loop {
using namespace pixel_engine;
using namespace prelude;

void print_hello(Command command) { std::cout << "Hello, World!" << std::endl; }

int count_down = 20;

bool should_call_exit() {
    count_down--;
    return count_down <= 0;
}

void exit_app(EventWriter<AppExit> exit_event) {
    if ((count_down--) <= 0) exit_event.write(AppExit{});
}

class LoopTestPlugin : public Plugin {
   public:
    void build(App& app) override {
        app.add_system(print_hello)
            .in_stage(app::Update)
            ->add_system(exit_app)
            .in_stage(app::Update);
    }
};

void test() {
    App app;
    app.enable_loop().add_plugin(LoopTestPlugin{}).run();
}
}  // namespace test_loop