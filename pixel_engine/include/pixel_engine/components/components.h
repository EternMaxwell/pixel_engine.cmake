#pragma once

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace core_components {
        struct Transform {
            glm::vec3 translation = glm::vec3(0.0f);
            glm::quat quaternion = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 scale = glm::vec3(1.0f);

            void translate(const glm::vec3& translation) { this->translation += translation; }
            void rotate(const glm::vec3& axis, float angle) { quaternion = glm::rotate(quaternion, angle, axis); }
            void rotate_x(float angle) { quaternion = glm::rotate(quaternion, angle, glm::vec3(1.0f, 0.0f, 0.0f)); }
            void rotate_y(float angle) { quaternion = glm::rotate(quaternion, angle, glm::vec3(0.0f, 1.0f, 0.0f)); }
            void rotate_z(float angle) { quaternion = glm::rotate(quaternion, angle, glm::vec3(0.0f, 0.0f, 1.0f)); }
            void scale_by(const glm::vec3& scale) { this->scale *= scale; }
            void scale_x(float scale) { this->scale.x *= scale; }
            void scale_y(float scale) { this->scale.y *= scale; }
            void scale_z(float scale) { this->scale.z *= scale; }
            void get_matrix(glm::mat4* matrix) const {
                *matrix = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(quaternion) *
                          glm::scale(glm::mat4(1.0f), scale);
            }
        };

        struct Projection {
            float fov = 45.0f;
            float aspect = 1.0f;
            float near = 0.1f;
            float far = 100.0f;

            void get_matrix(glm::mat4* matrix) const { *matrix = glm::perspective(fov, aspect, near, far); }
        };

        struct OrthoProjection {
            float left = -1.0f;
            float right = 1.0f;
            float bottom = -1.0f;
            float top = 1.0f;
            float near = -1.0f;
            float far = 1.0f;

            void get_matrix(glm::mat4* matrix) const { *matrix = glm::ortho(left, right, bottom, top, near, far); }
        };

        struct Camera2d {};

        struct Camera2dBundle {
            entity::Bundle bundle;
            Transform transform = Transform();
            OrthoProjection projection = OrthoProjection();
            Camera2d camera;
        };
    }  // namespace core_components
}  // namespace pixel_engine