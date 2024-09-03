#pragma once

#include <memory>

namespace pixel_engine {
namespace app {
template <typename ResT>
struct Resource {
   private:
    std::shared_ptr<ResT> m_res;

   public:
    Resource(std::shared_ptr<void> resource)
        : m_res(std::static_pointer_cast<ResT>(resource)) {}
    Resource() : m_res(nullptr) {}

    /**
     * @brief Check if the resource has a value.
     */
    bool has_value() { return m_res != nullptr; }

    ResT& operator*() { return *m_res; }
    ResT* operator->() { return m_res.get(); }
};
}  // namespace app
}  // namespace pixel_engine