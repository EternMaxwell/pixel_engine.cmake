#pragma once

#include "pixel_engine/entity.h"
#include "pixel_engine/transform/components.h"

namespace pixel_engine {
    namespace camera {
        namespace components {
            using namespace transform;

            struct Camera2d {};

            struct Camera2dBundle {
                entity::Bundle bundle;
                Transform transform = Transform();
                OrthoProjection projection = OrthoProjection();
                Camera2d camera;
            };

            struct Camera3d {};

            struct Camera3dBundle {
                entity::Bundle bundle;
                Transform transform = Transform();
                Projection projection = Projection();
                Camera3d camera;
            };

            struct CameraPath {
                glm::vec3 position;
                glm::vec3 direction;
                glm::vec3 up;

                void set_position(glm::vec3 position) { this->position = position; }
                void look_at(glm::vec3 direction) { this->direction = direction; }
                void set_up(glm::vec3 up) { this->up = up; }
                void rotate(glm::vec3 axis, float angle) {
                    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, axis);
                    direction = glm::vec3(rotation * glm::vec4(direction, 1.0f));
                    up = glm::vec3(rotation * glm::vec4(up, 1.0f));
                }
            };
        }  // namespace components

        using Camera2d = components::Camera2d;
        using Camera2dBundle = components::Camera2dBundle;
        using Camera3d = components::Camera3d;
        using Camera3dBundle = components::Camera3dBundle;
        using CameraPath = components::CameraPath;
    }  // namespace camera
}  // namespace pixel_engine