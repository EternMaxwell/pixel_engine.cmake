#pragma once

#include <pixel_engine/app.h>
#include <pixel_engine/render_vk.h>

#include <glm/glm.hpp>

namespace pixel_engine {
namespace sprite {
namespace resources {
using namespace prelude;
using namespace render_vk::components;

struct SpriteServerVK {
    std::unordered_map<std::string, Handle<ImageView>> images;
    // The entity the handle points to will have image, image view, image index.
    Handle<ImageView> load_image(Command& cmd, const std::string& _path);
};
}  // namespace resources
}  // namespace sprite
}  // namespace pixel_engine