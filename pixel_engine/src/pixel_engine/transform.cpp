#include <glm/ext/matrix_clip_space.hpp>

#include "pixel_engine/transform/components.h"

void pixel_engine::transform::components::Transform::translate(const glm::vec3& translation) {
    this->translation += translation;
}
void pixel_engine::transform::components::Transform::from_xyz(const glm::vec3& xyz) { translation = -xyz; }
void pixel_engine::transform::components::Transform::rotate(const glm::vec3& axis, float angle) {
    quaternion = glm::rotate(quaternion, angle, axis);
}
void pixel_engine::transform::components::Transform::rotate_x(float angle) {
    quaternion = glm::rotate(quaternion, angle, glm::vec3(1.0f, 0.0f, 0.0f));
}
void pixel_engine::transform::components::Transform::rotate_y(float angle) {
    quaternion = glm::rotate(quaternion, angle, glm::vec3(0.0f, 1.0f, 0.0f));
}
void pixel_engine::transform::components::Transform::rotate_z(float angle) {
    quaternion = glm::rotate(quaternion, angle, glm::vec3(0.0f, 0.0f, 1.0f));
}
void pixel_engine::transform::components::Transform::scale_by(const glm::vec3& scale) { this->scale *= scale; }
void pixel_engine::transform::components::Transform::scale_x(float scale) { this->scale.x *= scale; }
void pixel_engine::transform::components::Transform::scale_y(float scale) { this->scale.y *= scale; }
void pixel_engine::transform::components::Transform::scale_z(float scale) { this->scale.z *= scale; }
void pixel_engine::transform::components::Transform::look_at(const glm::vec3& target, const glm::vec3& up) {
    quaternion = glm::quatLookAt(glm::normalize(target + translation), up);
}
void pixel_engine::transform::components::Transform::get_matrix(glm::mat4* matrix) const {
    *matrix =
        glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(quaternion) * glm::scale(glm::mat4(1.0f), scale);
}

void pixel_engine::transform::components::Projection::get_matrix(glm::mat4* matrix) const {
    *matrix = glm::perspective(fov, aspect, near, far);
}

void pixel_engine::transform::components::OrthoProjection::get_matrix(glm::mat4* matrix) const {
    *matrix = glm::ortho(left, right, bottom, top, near, far);
}
