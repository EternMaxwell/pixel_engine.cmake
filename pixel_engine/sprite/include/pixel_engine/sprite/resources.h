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
    SpriteServerVK() = default;
    SpriteServerVK(const SpriteServerVK&) = delete;
    SpriteServerVK(SpriteServerVK&&)      = default;
    SpriteServerVK& operator=(const SpriteServerVK&) = delete;
    SpriteServerVK& operator=(SpriteServerVK&&) = default;

    spp::sparse_hash_map<std::string, Entity> images;
    std::deque<int> free_image_indices;
    int next_image_index = 0;
    spp::sparse_hash_map<std::string, Entity> samplers;
    std::deque<int> free_sampler_indices;
    int next_sampler_index = 0;
    // The entity the handle points to will have image, image view, image index.
    Entity load_image(Command& cmd, const std::string& _path);
    Entity create_sampler(
        Command& cmd, vk::SamplerCreateInfo create_info, const std::string& name
    );
    Entity get_sampler(const std::string& name);
};
}  // namespace resources
}  // namespace sprite
}  // namespace pixel_engine