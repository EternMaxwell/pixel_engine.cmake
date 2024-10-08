#pragma once

namespace pixel_engine {
namespace fg {
class resource {
   public:
    resource();
    ~resource();

   private:
    size_t id;
    size_t ref_count;
};
class FrameGraph {
   public:
    FrameGraph();
    ~FrameGraph();

   private:
};
}  // namespace fg
}  // namespace pixel_engine