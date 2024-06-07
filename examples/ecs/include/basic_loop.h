#include <pixel_engine/entity.h>

namespace test_loop {
    using namespace pixel_engine;

    void print_hello(entity::Command command) { std::cout << "Hello, World!" << std::endl; }

    int count_down = 20;

    bool should_call_exit() {
        count_down--;
        return count_down <= 0;
    }

    void exit_app(entity::EventWriter<entity::AppExit> exit_event) {
        if ((count_down--) <= 0) exit_event.write(entity::AppExit{});
    }

    class LoopTestPlugin : public entity::Plugin {
       public:
        void build(entity::App& app) override {
            app.add_system(entity::Update{}, print_hello).add_system(entity::Update{}, exit_app);
        }
    };

    void test() {
        entity::App app;
        app.add_plugin(entity::LoopPlugin{}).add_plugin(LoopTestPlugin{}).run();
    }
}  // namespace test_loop