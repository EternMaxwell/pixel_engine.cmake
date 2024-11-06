#include "pixel_engine/app/tools.h"

using namespace pixel_engine::app;
using namespace pixel_engine::app_tools;

Entity& Entity::operator=(entt::entity id) {
    this->id = id;
    return *this;
}
Entity::operator entt::entity() { return id; }
Entity::operator bool() { return id != entt::null; }
bool Entity::operator!() { return id == entt::null; }
bool Entity::operator==(const Entity& other) { return id == other.id; }
bool Entity::operator!=(const Entity& other) { return id != other.id; }
bool Entity::operator==(const entt::entity& other) { return id == other; }
bool Entity::operator!=(const entt::entity& other) { return id != other; }

size_t std::hash<Entity>::operator()(const Entity& entity) const {
    return std::hash<entt::entity>()(entity.id);
}
bool std::equal_to<Entity>::operator()(const Entity& a, const Entity& b) const {
    return a.id == b.id;
}