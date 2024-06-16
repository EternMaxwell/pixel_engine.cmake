#pragma once

#include <glad/glad.h>

#include "resources.h"

namespace pixel_engine {
    namespace asset_server_gl {
        using namespace prelude;

        class AssetServerGLPlugin : public entity::Plugin {
           private:
            std::string m_base_path = "./";
            std::function<void(Command)> insert_asset_server = [&](Command command) {
                using namespace asset_server_gl::resources;
                command.insert_resource(AssetServerGL(m_base_path));
            };

           public:
            SystemNode insert_asset_server_node;

            void set_base_path(const std::string& base_path) { m_base_path = base_path; }

            void build(App& app) override {
                using namespace asset_server_gl;
                app.add_system(Startup{}, insert_asset_server, &insert_asset_server_node);
            }
        };
    }  // namespace asset_server_gl
}  // namespace pixel_engine