#include "pixel_engine/font.h"

using namespace pixel_engine::font::systems;

EPIX_API void ogl::create_text_renderer(Command command) {
    command.insert_resource(components::TextRendererGl{});
}