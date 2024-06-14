#pragma once

#include <any>
#include <deque>
#include <entt/entity/registry.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <pfr.hpp>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_map>

namespace pixel_engine {
    namespace entity {
        template <typename ResT>
        class Resource {
           private:
            std::shared_ptr<ResT> m_res;

           public:
            Resource(std::any* resource) : m_res(std::any_cast<std::shared_ptr<ResT>&>(*resource)) {}
            Resource() : m_res(nullptr) {}

            /*! @brief Check if the resource is const.
             * @return True if the resource is const, false otherwise.
             */
            constexpr bool is_const() { return std::is_const_v<std::remove_reference_t<ResT>>; }

            /*! @brief Check if the resource has a value.
             * @return True if the resource has a value, false otherwise.
             */
            bool has_value() { return m_res != nullptr; }

            ResT& operator*() { return *m_res; }
            ResT* operator->() { return m_res.get(); }
        };
    }  // namespace entity
}  // namespace pixel_engine