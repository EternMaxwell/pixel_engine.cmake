#include <pixel_engine/entity.h>

namespace test_loop {
    using namespace pixel_engine;

    void print_hello(entity::Command command) {
        std::cout << "Hello, World!" << std::endl;
    }

    int count_down = 20;

    bool should_call_exit(entity::Command) {
        count_down--;
        return count_down <= 0;
    }

    class LoopTestPlugin : public entity::Plugin {
       public:
        void build(entity::App& app) override {
            app.add_system(entity::Update{}, print_hello)
                .add_system(entity::Update{}, entity::exit_app, should_call_exit);
        }
    };

    void test() {
        entity::App app;
        app.add_plugin(entity::LoopPlugin{}).add_plugin(LoopTestPlugin{}).run();
    }
}  // namespace test_loop