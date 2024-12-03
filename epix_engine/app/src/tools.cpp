#include "pixel_engine/app/tools.h"

using namespace pixel_engine::app;
using namespace pixel_engine::app_tools;

EPIX_API Entity& Entity::operator=(entt::entity id) {
    this->id = id;
    return *this;
}
EPIX_API Entity::operator entt::entity() { return id; }
EPIX_API Entity::operator bool() { return id != entt::null; }
EPIX_API bool Entity::operator!() { return id == entt::null; }
EPIX_API bool Entity::operator==(const Entity& other) { return id == other.id; }
EPIX_API bool Entity::operator!=(const Entity& other) { return id != other.id; }
EPIX_API bool Entity::operator==(const entt::entity& other) { return id == other; }
EPIX_API bool Entity::operator!=(const entt::entity& other) { return id != other; }

EPIX_API size_t std::hash<Entity>::operator()(const Entity& entity) const {
    return std::hash<entt::entity>()(entity.id);
}
EPIX_API bool std::equal_to<Entity>::operator()(
    const Entity& a, const Entity& b
) const {
    return a.id == b.id;
}