#include <pixel_engine/entity/entity.h>

#include <format>
#include <iomanip>
#include <iostream>
#include <random>

namespace test_resource {
    using namespace pixel_engine;

    struct Resource {
        int data;
    };

    void access_resource(entity::Resource<Resource> resource) {
        std::cout << "access_resource" << std::endl;
        if (resource.has_value()) {
            std::cout << "resource: " << resource.value().data << std::endl;
        } else {
            std::cout << "resource: null" << std::endl;
        }
        std::cout << std::endl;
    }

    void set_resource(entity::Command command) {
        std::cout << "set_resource" << std::endl;
        command.insert_resource(Resource{.data = 100});
        std::cout << std::endl;
    }

    void set_resource_with_init(entity::Command command) {
        std::cout << "set_resource_with_init" << std::endl;
        command.init_resource<Resource>();
        std::cout << std::endl;
    }

    void remove_resource(entity::Command command) {
        std::cout << "remove_resource" << std::endl;
        command.remove_resource<Resource>();
        std::cout << std::endl;
    }

    void change_resource(entity::Resource<Resource> resource) {
        std::cout << "change_resource" << std::endl;
        if (resource.has_value()) {
            resource.value().data = 200;
        }
        std::cout << std::endl;
    }

    class ResourceTestPlugin : public entity::Plugin {
       public:
        void build(entity::App& app) override {
            app.add_system(entity::Startup{}, set_resource)
                .add_system(entity::Startup{}, access_resource)
                .add_system(entity::Startup{}, remove_resource)
                .add_system(entity::Startup{}, set_resource_with_init)
                .add_system(entity::Startup{}, access_resource)
                .add_system(entity::Startup{}, change_resource)
                .add_system(entity::Startup{}, access_resource)
                .add_system(entity::Startup{}, remove_resource);
        }
    };

    void test() {
        entity::App app;
        app.add_plugin(ResourceTestPlugin{}).run();
    }
}  // namespace test_resource