#include "pixel_engine/font.h"

using namespace pixel_engine::font::systems;

void ogl::create_text_renderer(Command command) {
    command.insert_resource(components::TextRendererGl{});
}