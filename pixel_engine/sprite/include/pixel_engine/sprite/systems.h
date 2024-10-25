#pragma once

#include "components.h"
#include "resources.h"

namespace pixel_engine {
namespace sprite {
namespace systems {
using namespace prelude;
using namespace render_vk::components;
using namespace sprite::components;
using namespace sprite::resources;

void insert_sprite_server_vk(Command cmd);  // called at startup stage

void loading_actual_image(
    Command cmd,
    Query<Get<Entity, ImageLoading>> query,
    Query<Get<Device, CommandPool, Queue>, With<RenderContext>> ctx_query,
    Resource<SpriteServerVK> sprite_server
);  // called at render prepare stage

void creating_actual_sampler(
    Command cmd,
    Query<Get<Entity, SamplerCreating>> query,
    Query<Get<Device>, With<RenderContext>> ctx_query,
    Resource<SpriteServerVK> sprite_server
);  // called at render prepare stage

void create_sprite_renderer_vk(
    Command cmd,
    Query<Get<Device, CommandPool, Queue>, With<RenderContext>> ctx_query
);  // called at startup stage

void destroy_sprite_renderer_vk(
    Query<Get<SpriteRenderer>> renderer_query,
    Query<Get<Device>, With<RenderContext>> query
);  // called at exit stage

void destroy_sprite_server_vk_images(
    Resource<SpriteServerVK> sprite_server,
    Query<Get<Image, ImageView>, With<ImageIndex>> query,
    Query<Get<Device, CommandPool, Queue>, With<RenderContext>> ctx_query
);  // called at exit stage

void destroy_sprite_server_vk_samplers(
    Resource<SpriteServerVK> sprite_server,
    Query<Get<Sampler>, With<SamplerIndex>> query,
    Query<Get<Device>, With<RenderContext>> ctx_query
);  // called at exit stage

}  // namespace systems
}  // namespace sprite
}  // namespace pixel_engine