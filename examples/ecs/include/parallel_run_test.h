#include <pixel_engine/entity.h>

using namespace pixel_engine;

namespace test_parallel_run {
    using namespace entity;

    struct Data1 {
        int data;
    };

    struct Data2 {
        int data;
    };

    std::mutex mutex;

    int loops = 100000;

    void spawn_data1(Command command) {
        for (int i = 0; i < 100; i++) command.spawn(Data1{.data = 1});
    }

    void spawn_data2(Command command) {
        for (int i = 0; i < 100; i++) command.spawn(Data2{.data = 2});
    }

    void print_data1(Query<std::tuple<Data1>, std::tuple<>> query) {
        for (auto [data] : query.iter()) {
            std::cout << "Data1: " << data.data++ << std::endl;
        }
        std::cout << "Data1: End" << std::endl;
    }

    void print_data2(Query<std::tuple<Data2>, std::tuple<>> query) {
        for (auto [data] : query.iter()) {
            std::cout << "Data2: " << data.data++ << std::endl;
        }
        std::cout << "Data2: End" << std::endl;
    }

    void print_endl() {
        std::cout << std::endl;
    }

    void exit(EventWriter<AppExit> exit_event) {
        if (!loops--) exit_event.write(AppExit{});
    }

    class ParallelTestPlugin : public entity::Plugin {
       public:
        void build(entity::App& app) override {
            Node node1, node2;
            app.add_plugin(LoopPlugin{})
                .add_system(Startup{}, spawn_data1)
                .add_system(Startup{}, spawn_data2)
                .add_system(Update{}, print_data1, &node1)
                .add_system(Update{}, print_data2, &node2)
                .add_system(Update{}, print_endl, node1, node2)
                .add_system(Update{}, exit);
        }
    };

    void test() {
        spdlog::set_level(spdlog::level::debug);
        entity::App app;
        app.add_plugin(ParallelTestPlugin{}).run_parallel();
    }
}  // namespace test_parallel_run