#include <pixel_engine/entity/entity.h>

#include <format>
#include <iomanip>
#include <iostream>
#include <random>

namespace test_event {
    using namespace pixel_engine;

    struct TestEvent {
        int data;
    };

    void write_event(entity::EventWriter<TestEvent> event) {
        std::cout << "write_event" << std::endl;
        event.write(TestEvent{.data = 100}).write(TestEvent{.data = 200});
        std::cout << std::endl;
    }

    void read_event(entity::EventReader<TestEvent> event) {
        std::cout << "read_event" << std::endl;
        for (auto e : event.read()) {
            std::cout << "event: " << e.data << std::endl;
        }
        std::cout << std::endl;
    }

    void clear_event(entity::Command command) {
        std::cout << "clear_event" << std::endl;
        command.clear_events<TestEvent>();
        std::cout << std::endl;
    }

    bool check_if_event_exist(entity::EventReader<TestEvent> event) {
        std::cout << "check_if_event_exist" << std::endl;
        bool exist = !event.empty();
        std::cout << "exist: " << std::boolalpha << exist << std::endl;
        std::cout << std::endl;
        return exist;
    }

    class EventTestPlugin : public entity::Plugin {
       public:
        void build(entity::App& app) override {
            app.add_system(entity::Startup{}, write_event)
                .add_system(entity::Startup{}, read_event)
                .add_system(entity::Startup{}, clear_event)
                .add_system(entity::Startup{}, read_event, check_if_event_exist)
                .add_system(entity::Startup{}, write_event)
                .add_system(entity::Startup{}, read_event,
                            check_if_event_exist);
        }
    };

    void test() {
        entity::App app;
        app.add_plugin(EventTestPlugin{}).run();
    }
}  // namespace test_event