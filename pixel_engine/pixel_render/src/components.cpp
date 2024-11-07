#include "pixel_engine/render/pixel/components.h"

namespace pixel_engine::render::pixel {
namespace components {
PixelBlock PixelBlock::create(glm::uvec2 size) {
    PixelBlock block;
    block.size = size;
    block.pixels.resize(size.x * size.y);
    return block;
}
glm::vec4& PixelBlock::operator[](glm::uvec2 pos) {
    return pixels[pos.x + pos.y * size.x];
}
const glm::vec4& PixelBlock::operator[](glm::uvec2 pos) const {
    return pixels[pos.x + pos.y * size.x];
}
}  // namespace components
}  // namespace pixel_engine::render::pixel