#include <spdlog/spdlog.h>

#include "pixel_engine/world/sand/components.h"

using namespace pixel_engine::world::sand::components;

static std::shared_ptr<spdlog::logger> elem_registry_logger =
    spdlog::default_logger()->clone("elem_registry");

EPIX_API ElemRegistry::ElemRegistry() {}
EPIX_API ElemRegistry::ElemRegistry(const ElemRegistry& other)
    : elemId_map(other.elemId_map), elements(other.elements) {}
EPIX_API ElemRegistry::ElemRegistry(ElemRegistry&& other)
    : elemId_map(std::move(other.elemId_map)),
      elements(std::move(other.elements)) {}
EPIX_API ElemRegistry& ElemRegistry::operator=(const ElemRegistry& other) {
    elemId_map = other.elemId_map;
    elements   = other.elements;
    return *this;
}
EPIX_API ElemRegistry& ElemRegistry::operator=(ElemRegistry&& other) {
    elemId_map = std::move(other.elemId_map);
    elements   = std::move(other.elements);
    return *this;
}

EPIX_API int ElemRegistry::register_elem(
    const std::string& name, const Element& elem
) {
    std::lock_guard<std::mutex> lock(mutex);
    uint32_t id      = elements.size();
    elemId_map[name] = id;
    elements.emplace_back(elem);
    return id;
}
EPIX_API int ElemRegistry::elem_id(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex);
    return elemId_map.at(name);
}
EPIX_API const Element& ElemRegistry::get_elem(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex);
    return elements.at(elemId_map.at(name));
}
EPIX_API const Element& ElemRegistry::get_elem(int id) const {
    return elements[id];
}
EPIX_API const Element& ElemRegistry::operator[](int id) const {
    return get_elem(id);
}
EPIX_API void ElemRegistry::add_equiv(
    const std::string& name, const std::string& equiv
) {
    std::lock_guard<std::mutex> lock(mutex);
    if (elemId_map.find(name) == elemId_map.end()) {
        elem_registry_logger->warn(
            "Attempted to add equivalent element {} to non-existent "
            "element {}",
            equiv, name
        );
        return;
    }
    elemId_map[equiv] = elemId_map.at(name);
}