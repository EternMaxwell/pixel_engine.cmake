#include "render_vk/components.h"
#include "render_vk/resources.h"
#include "render_vk/systems.h"
#include "render_vk/vulkan/device.h"
#include "render_vk/vulkan_headers.h"

namespace pixel_engine {
namespace render_vk {
using namespace prelude;
struct RenderVKPlugin : Plugin {
    void build(App& app) override {
        app.add_system(PreStartup(), systems::create_context)
            .in_set(window::WindowStartUpSets::after_window_creation);
        app.add_system(Prepare(), systems::recreate_swap_chain);
        app.add_system(Prepare(), systems::get_next_image)
            .after(systems::recreate_swap_chain);
        app.add_system(PostRender(), systems::present_frame);
        app.add_system(PostShutdown(), systems::destroy_context);
    }
};
}  // namespace render_vk
}  // namespace pixel_engine
