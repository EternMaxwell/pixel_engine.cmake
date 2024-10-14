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

void write_event_s(EventWriter<TestEvent> event) {
    std::cout << "write_event" << std::endl;
    event.write(TestEvent{.data = 100}).write(TestEvent{.data = 200});
    std::cout << std::endl;
}

void read_event_s(EventReader<TestEvent> event) {
    std::cout << "read_event" << std::endl;
    for (auto& e : event.read()) {
        std::cout << "event: " << e.data << std::endl;
    }
    std::cout << std::endl;
}

void write_event_u(EventWriter<TestEvent> event) {
    std::cout << "write_event" << std::endl;
    event.write(TestEvent{.data = 100}).write(TestEvent{.data = 200});
    std::cout << std::endl;
}

void read_event_u(EventReader<TestEvent> event) {
    std::cout << "read_event" << std::endl;
    for (auto& e : event.read()) {
        std::cout << "event: " << e.data << std::endl;
    }
    std::cout << std::endl;
}

void read_event_u2(EventReader<TestEvent> event) {
    std::cout << "read_event" << std::endl;
    for (auto& e : event.read()) {
        std::cout << "event: " << e.data << std::endl;
    }
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
        app.add_system(write_event_s)
            .in_stage(app::Startup)
            ->add_system(read_event_s)
            .in_stage(app::Startup)
            .after(write_event_s)
            ->add_system(read_event_u)
            .in_stage(app::Update)
            ->add_system(write_event_u)
            .in_stage(app::Update)
            .after(read_event_u)
            ->add_system(read_event_u2)
            .in_stage(app::Update)
            .after(write_event_u);
    }
};

void test() {
    App app;
    app.add_plugin(EventTestPlugin{}).run();
}
}  // namespace test_event