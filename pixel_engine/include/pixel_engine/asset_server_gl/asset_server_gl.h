#pragma once

#include <glad/glad.h>

#include "resources.h"

namespace pixel_engine {
    namespace asset_server_gl {
        using namespace prelude;

        enum class AssetServerGLSets {
            insert_asset_server,
            after_insertion,
        };

        class AssetServerGLPlugin : public entity::Plugin {
           private:
            std::string m_base_path = "./";
            std::function<void(Command)> insert_asset_server = [&](Command command) {
                using namespace asset_server_gl::resources;
                command.insert_resource(AssetServerGL(m_base_path));
            };

           public:
            void set_base_path(const std::string& base_path) { m_base_path = base_path; }

            void build(App& app) override;
        };
    }  // namespace asset_server_gl
}  // namespace pixel_engine