#include "epix/font.h"

using namespace epix::font::systems;

EPIX_API void ogl::create_text_renderer(Command command) {
    command.insert_resource(components::TextRendererGl{});
}