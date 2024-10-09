#pragma once

#include <memory>
#include <stack>
#include <vector>
#include <unordered_map>
#include <string>

namespace pixel_engine {
namespace fg {
struct resource_base {
   public:

   private:
    size_t id;
    size_t ref_count;
};
template <typename T>
struct resource : public resource_base {
   public:

   private:
    T data;
};
struct FrameGraph {
   public:

   private:
    std::vector<std::unique_ptr<resource_base>> resources;
    std::stack<size_t> free_resources;
    std::unordered_map<std::string, size_t> resource_map;

};
}  // namespace fg
}  // namespace pixel_engine