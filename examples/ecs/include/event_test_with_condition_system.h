#include <pixel_engine/entity.h>

#include <format>
#include <iomanip>
#include <iostream>
#include <random>

namespace test_event {
    using namespace pixel_engine;
    using namespace prelude;

    struct TestEvent {
        int data;
    };

    void write_event(EventWriter<TestEvent> event) {
        std::cout << "write_event" << std::endl;
        event.write(TestEvent{.data = 100}).write(TestEvent{.data = 200});
        std::cout << std::endl;
    }

    void read_event(EventReader<TestEvent> event) {
        std::cout << "read_event" << std::endl;
        for (auto& e : event.read()) {
            std::cout << "event: " << e.data << std::endl;
        }
        std::cout << std::endl;
    }

    void clear_event(Command command) {
        std::cout << "clear_event" << std::endl;
        command.clear_events<TestEvent>();
        std::cout << std::endl;
    }

    bool check_if_event_exist(EventReader<TestEvent> event) {
        std::cout << "check_if_event_exist" << std::endl;
        bool exist = !event.empty();
        std::cout << "exist: " << std::boolalpha << exist << std::endl;
        std::cout << std::endl;
        return exist;
    }

    class EventTestPlugin : public Plugin {
       public:
        void build(App& app) override {
            SystemNode node;
            app.add_system(Startup{}, write_event, &node)
                .add_system(Startup{}, read_event, &node, after(node))
                .add_system(Startup{}, clear_event, &node, after(node))
                .add_system(Update{}, read_event, &node, after(node))
                .add_system(Update{}, write_event, &node, after(node))
                .add_system(Update{}, read_event, &node, after(node));
        }
    };

    void test() {
        App app;
        app.add_plugin(EventTestPlugin{}).run();
    }
}  // namespace test_event