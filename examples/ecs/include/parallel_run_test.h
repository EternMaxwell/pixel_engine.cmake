#include <pixel_engine/entity.h>

using namespace pixel_engine;

namespace test_parallel_run {
    void system1() {
        for (int i = 0; i < 10; i++) {
            std::cout << "System 1: " << i << std::endl;
        }
    }

    void system2() {
        for (int i = 0; i < 10; i++) {
            std::cout << "System 2: " << i << std::endl;
        }
    }

    class ParallelTestPlugin : public entity::Plugin {
       public:
        void build(entity::App& app) override {
            std::shared_ptr<entity::SystemNode> node;
            app.add_system(entity::Startup{}, system1, &node)
                .add_system(entity::Startup{}, system2);
        }
    };

    void test() {
        entity::App app;
        app.add_plugin(ParallelTestPlugin{}).run_parallel();
    }
}  // namespace test_parallel_run