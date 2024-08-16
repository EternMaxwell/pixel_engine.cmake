#include <pixel_engine/entity.h>

#include <format>
#include <iomanip>
#include <iostream>
#include <random>

namespace test_resource {
using namespace pixel_engine;
using namespace prelude;

struct Res {
    int data;
};

void access_resource(Resource<Res> resource) {
    std::cout << "access_resource" << std::endl;
    if (resource.has_value()) {
        std::cout << "resource: " << resource->data << std::endl;
    } else {
        std::cout << "resource: null" << std::endl;
    }
    std::cout << std::endl;
}

void set_resource(Command command) {
    std::cout << "set_resource" << std::endl;
    command.insert_resource(Res{.data = 100});
    std::cout << std::endl;
}

void set_resource_with_init(Command command) {
    std::cout << "set_resource_with_init" << std::endl;
    command.init_resource<Res>();
    std::cout << std::endl;
}

void remove_resource(Command command) {
    std::cout << "remove_resource" << std::endl;
    command.remove_resource<Res>();
    std::cout << std::endl;
}

void change_resource(Resource<Res> resource) {
    std::cout << "change_resource" << std::endl;
    resource->data = 200;
    std::cout << std::endl;
}

class ResourceTestPlugin : public entity::Plugin {
   public:
    void build(App& app) override {
        SystemNode node;
        app.add_system(Startup{}, set_resource, &node)
            .add_system(Startup{}, access_resource, &node, after(node))
            .add_system(Startup{}, remove_resource, &node, after(node))
            .add_system(Startup{}, set_resource_with_init, &node, after(node))
            .add_system(Startup{}, access_resource, &node, after(node))
            .add_system(Startup{}, change_resource, &node, after(node))
            .add_system(Startup{}, access_resource, &node, after(node))
            .add_system(Startup{}, remove_resource, &node, after(node));
    }
};

void test() {
    entity::App app;
    app.add_plugin(ResourceTestPlugin{}).run();
}
}  // namespace test_resource