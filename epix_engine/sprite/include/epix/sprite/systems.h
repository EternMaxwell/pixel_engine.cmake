#pragma once

#include "components.h"
#include "resources.h"

namespace epix {
namespace sprite {
namespace systems {
using namespace prelude;
using namespace render_vk::components;
using namespace sprite::components;
using namespace sprite::resources;

EPIX_API void insert_sprite_server_vk(Command cmd);  // called at startup stage

EPIX_API void loading_actual_image(
    Command cmd,
    Query<Get<Entity, ImageLoading>> query,
    Query<Get<Device, CommandPool, Queue>, With<RenderContext>> ctx_query,
    ResMut<SpriteServerVK> sprite_server
);  // called at render prepare stage

EPIX_API void creating_actual_sampler(
    Command cmd,
    Query<Get<Entity, SamplerCreating>> query,
    Query<Get<Device>, With<RenderContext>> ctx_query,
    ResMut<SpriteServerVK> sprite_server
);  // called at render prepare stage

EPIX_API void create_sprite_renderer_vk(
    Command cmd,
    Query<Get<Device, CommandPool, Queue>, With<RenderContext>> ctx_query
);  // called at startup stage

EPIX_API void destroy_sprite_renderer_vk(
    Query<Get<SpriteRenderer>> renderer_query,
    Query<Get<Device>, With<RenderContext>> query
);  // called at exit stage

EPIX_API void update_image_bindings(
    Query<Get<ImageBindingUpdate>> query,
    Query<Get<ImageView, ImageIndex>, With<Image>> image_query,
    Query<Get<SpriteRenderer>> renderer_query,
    Query<Get<Device>, With<RenderContext>> ctx_query
);  // called at render prepare stage

EPIX_API void update_sampler_bindings(
    Query<Get<SamplerBindingUpdate>> query,
    Query<Get<Sampler, SamplerIndex>> sampler_query,
    Query<Get<SpriteRenderer>> renderer_query,
    Query<Get<Device>, With<RenderContext>> ctx_query
);  // called at render prepare stage

EPIX_API void create_sprite_depth_vk(
    Command cmd,
    Query<Get<Device, CommandPool, Queue, Swapchain>, With<RenderContext>>
        ctx_query,
    Query<Get<Image, ImageView>, With<SpriteDepth, SpriteDepthExtent>> query
);  // called at post startup stage in case of user assigned depth image

EPIX_API void update_sprite_depth_vk(
    Query<Get<ImageView, Image, SpriteDepthExtent>, With<SpriteDepth>> query,
    Query<Get<Device, Swapchain>, With<RenderContext>> ctx_query
);  // called at render prerender stage

EPIX_API void destroy_sprite_depth_vk(
    Query<Get<Device>, With<RenderContext>> query,
    Query<Get<ImageView, Image>, With<SpriteDepth, SpriteDepthExtent>>
        depth_query
);  // called at exit stage

EPIX_API void draw_sprite_2d_vk(
    Query<Get<SpriteRenderer>> renderer_query,
    Query<Get<Sprite, SpritePos2D>> sprite_query,
    Query<Get<Device, CommandPool, Queue, Swapchain>, With<RenderContext>>
        ctx_query,
    Query<Get<ImageIndex>, With<Image, ImageView>> image_query,
    Query<Get<SamplerIndex>, With<Sampler>> sampler_query,
    Query<Get<ImageView>, With<SpriteDepth, Image, SpriteDepthExtent>>
        depth_query
);  // called at render stage

EPIX_API void destroy_sprite_server_vk_images(
    ResMut<SpriteServerVK> sprite_server,
    Query<Get<Image, ImageView>, With<ImageIndex>> query,
    Query<Get<Device, CommandPool, Queue>, With<RenderContext>> ctx_query
);  // called at exit stage

EPIX_API void destroy_sprite_server_vk_samplers(
    ResMut<SpriteServerVK> sprite_server,
    Query<Get<Sampler>, With<SamplerIndex>> query,
    Query<Get<Device>, With<RenderContext>> ctx_query
);  // called at exit stage

}  // namespace systems
}  // namespace sprite
}  // namespace epix