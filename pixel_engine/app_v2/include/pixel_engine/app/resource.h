#pragma once

#include <memory>

namespace pixel_engine {
namespace app {
template <typename ResT>
struct Res {
   private:
    const ResT* m_res;

   public:
    Resource(void* resource) : m_res(resource) {}
    Resource() : m_res(nullptr) {}

    /**
     * @brief Check if the resource has a value.
     */
    bool has_value() { return m_res != nullptr; }

    const ResT& operator*() { return *m_res; }
    const ResT* operator->() { return m_res; }
};

template <typename ResT>
struct ResMut {
   private:
    ResT* m_res;

   public:
    ResourceMut(void* resource) : m_res(resource) {}
    ResourceMut() : m_res(nullptr) {}

    /**
     * @brief Check if the resource has a value.
     */
    bool has_value() { return m_res != nullptr; }

    ResT& operator*() { return *m_res; }
    ResT* operator->() { return m_res; }
};
}  // namespace app
}  // namespace pixel_engine