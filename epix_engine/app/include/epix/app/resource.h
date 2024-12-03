#pragma once

#include <memory>

namespace epix {
namespace app {
template <typename ResT>
struct Res {
   private:
    const ResT* m_res;

   public:
    Res(void* resource) : m_res(static_cast<const ResT*>(resource)) {}
    Res() : m_res(nullptr) {}

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
    ResMut(void* resource) : m_res(static_cast<ResT*>(resource)) {}
    ResMut() : m_res(nullptr) {}

    /**
     * @brief Check if the resource has a value.
     */
    bool has_value() { return m_res != nullptr; }

    ResT& operator*() { return *m_res; }
    ResT* operator->() { return m_res; }
};
}  // namespace app
}  // namespace epix