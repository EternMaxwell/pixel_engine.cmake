#include "epix/physics2d/utils.h"

using namespace epix::physics2d;
using namespace epix::physics2d::utils;

binary_grid utils::operator&(const binary_grid& lhs, const binary_grid& rhs) {
    binary_grid result(lhs.width, lhs.height);
    for (int i = 0; i < lhs.pixels.size(); i++) {
        result.pixels[i] = lhs.pixels[i] & rhs.pixels[i];
    }
    return result;
}

binary_grid utils::operator|(const binary_grid& lhs, const binary_grid& rhs) {
    binary_grid result(lhs.width, lhs.height);
    for (int i = 0; i < lhs.pixels.size(); i++) {
        result.pixels[i] = lhs.pixels[i] | rhs.pixels[i];
    }
    return result;
}

binary_grid utils::operator^(const binary_grid& lhs, const binary_grid& rhs) {
    binary_grid result(lhs.width, lhs.height);
    for (int i = 0; i < lhs.pixels.size(); i++) {
        result.pixels[i] = lhs.pixels[i] ^ rhs.pixels[i];
    }
    return result;
}

binary_grid utils::operator~(const binary_grid& lhs) {
    binary_grid result(lhs.width, lhs.height);
    for (int i = 0; i < lhs.pixels.size(); i++) {
        result.pixels[i] = ~lhs.pixels[i];
    }
    return result;
}

binary_grid utils::binary_grid::sub(int x, int y, int w, int h) const {
    binary_grid result(w, h);
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            result.set(i, j, contains(x + i, y + j));
        }
    }
    return result;
}

constexpr int size = (9 >> 5) + (9 & 31 ? 1 : 0);

binary_grid::binary_grid(int width, int height)
    : width(width),
      height(height),
      column((width >> 5) + (width & 31 ? 1 : 0)) {
    pixels.resize(column * height);
}

void binary_grid::set(int x, int y, bool value) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    int index = (x >> 5) + y * column;
    int bit   = x & 31;
    if (value)
        pixels[index] |= 1 << bit;
    else
        pixels[index] &= ~(1 << bit);
}

bool binary_grid::contains(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    int index = (x >> 5) + y * column;
    int bit   = x & 31;
    return pixels[index] & (1 << bit);
}

glm::ivec2 binary_grid::size() const { return {width, height}; }

binary_grid& binary_grid::operator&=(const binary_grid& rhs) {
    for (int i = 0; i < pixels.size(); i++) {
        pixels[i] &= rhs.pixels[i];
    }
    return *this;
}

binary_grid& binary_grid::operator|=(const binary_grid& rhs) {
    for (int i = 0; i < pixels.size(); i++) {
        pixels[i] |= rhs.pixels[i];
    }
    return *this;
}

binary_grid& binary_grid::operator^=(const binary_grid& rhs) {
    for (int i = 0; i < pixels.size(); i++) {
        pixels[i] ^= rhs.pixels[i];
    }
    return *this;
}