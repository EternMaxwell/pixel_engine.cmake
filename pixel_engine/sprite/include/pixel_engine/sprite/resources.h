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
    std::deque<int> free_image_indices;
    int next_image_index = 0;
    std::unordered_map<std::string, Handle<Sampler>> samplers;
    std::deque<int> free_sampler_indices;
    int next_sampler_index = 0;
    // The entity the handle points to will have image, image view, image index.
    Handle<ImageView> load_image(Command& cmd, const std::string& _path);
    Handle<Sampler> create_sampler(
        Command& cmd, vk::SamplerCreateInfo create_info, const std::string& name
    );
    Handle<Sampler> get_sampler(const std::string& name);
};
}  // namespace resources
}  // namespace sprite
}  // namespace pixel_engine