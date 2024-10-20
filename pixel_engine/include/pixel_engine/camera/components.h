﻿#pragma once

#include "pixel_engine/entity.h"
#include "pixel_engine/transform/components.h"

namespace pixel_engine {
namespace camera {
namespace components {
using namespace transform;

struct Camera2d {};

struct Camera2dBundle : app::Bundle {
    Transform transform = Transform();
    OrthoProjection projection = OrthoProjection();
    Camera2d camera;

    auto unpack() {
        return std::forward_as_tuple(
            std::move(transform), std::move(projection), std::move(camera)
        );
    }
};
}  // namespace components

using Camera2d = components::Camera2d;
using Camera2dBundle = components::Camera2dBundle;
}  // namespace camera
}  // namespace pixel_engine