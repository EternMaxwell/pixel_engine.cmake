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

void update_image_bindings(
    Query<Get<ImageBindingUpdate>> query,
    Query<Get<ImageView, ImageIndex>, With<Image>> image_query,
    Query<Get<SpriteRenderer>> renderer_query,
    Query<Get<Device>, With<RenderContext>> ctx_query
);  // called at render prepare stage

void update_sampler_bindings(
    Query<Get<SamplerBindingUpdate>> query,
    Query<Get<Sampler, SamplerIndex>> sampler_query,
    Query<Get<SpriteRenderer>> renderer_query,
    Query<Get<Device>, With<RenderContext>> ctx_query
);  // called at render prepare stage

void create_sprite_depth_vk(
    Command cmd,
    Query<Get<Device, CommandPool, Queue, Swapchain>, With<RenderContext>>
        ctx_query,
    Query<Get<Image, ImageView>, With<SpriteDepth, SpriteDepthExtent>> query
);  // called at post startup stage in case of user assigned depth image

void update_sprite_depth_vk(
    Query<Get<ImageView, Image, SpriteDepthExtent>, With<SpriteDepth>> query,
    Query<Get<Device, Swapchain>, With<RenderContext>> ctx_query
);  // called at render prerender stage

void destroy_sprite_depth_vk(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<ImageView, Image>, With<SpriteDepth, SpriteDepthExtent>>
        depth_query
);  // called at exit stage

void draw_sprite_2d_vk(
    Query<Get<SpriteRenderer>> renderer_query,
    Query<Get<Sprite, SpritePos2D>> sprite_query,
    Query<Get<Device, CommandPool, Queue, Swapchain>, With<RenderContext>>
        ctx_query,
    Query<Get<ImageIndex>, With<Image, ImageView>> image_query,
    Query<Get<SamplerIndex>, With<Sampler>> sampler_query,
    Query<Get<ImageView>, With<SpriteDepth, Image, SpriteDepthExtent>> depth_query
);  // called at render stage

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