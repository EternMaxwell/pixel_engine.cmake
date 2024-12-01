#pragma once

#include <pixel_engine/app.h>

#include "asset_server_gl.h"
#include "resources.h"

namespace pixel_engine {
namespace asset_server_gl {
namespace systems {
using namespace prelude;

EPIX_API void insert_asset_server(
    Command command, ResMut<AssetServerGLPlugin> asset_server_gl
) {
    command.insert_resource(
        resources::AssetServerGL(asset_server_gl->get_base_path())
    );
}
}  // namespace systems
}  // namespace asset_server_gl
}  // namespace pixel_engine