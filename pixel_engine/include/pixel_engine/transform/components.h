#pragma once

#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace transform {
        namespace components {
            struct Transform {
                glm::vec3 translation = glm::vec3(0.0f);
                glm::quat quaternion = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                glm::vec3 scale = glm::vec3(1.0f);

                void translate(const glm::vec3& translation);
                void from_xyz(const glm::vec3& xyz);
                void rotate(const glm::vec3& axis, float angle);
                void rotate_x(float angle);
                void rotate_y(float angle);
                void rotate_z(float angle);
                void scale_by(const glm::vec3& scale);
                void scale_x(float scale);
                void scale_y(float scale);
                void scale_z(float scale);
                void look_at(const glm::vec3& target, const glm::vec3& up);
                void get_matrix(glm::mat4* matrix) const;
            };

            struct Projection {
                float fov = 45.0f;
                float aspect = 1.0f;
                float near = 0.1f;
                float far = 100.0f;

                void get_matrix(glm::mat4* matrix) const;
            };

            struct OrthoProjection {
                float left = -1.0f;
                float right = 1.0f;
                float bottom = -1.0f;
                float top = 1.0f;
                float near = -1.0f;
                float far = 1.0f;

                void get_matrix(glm::mat4* matrix) const;
            };
        }  // namespace components

        using Transform = components::Transform;
        using Projection = components::Projection;
        using OrthoProjection = components::OrthoProjection;
    }  // namespace transform
}  // namespace pixel_engine
