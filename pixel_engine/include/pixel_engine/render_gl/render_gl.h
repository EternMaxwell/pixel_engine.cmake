#pragma once

#include "components.h"
#include "systems.h"

namespace pixel_engine {
namespace render_gl {
using namespace prelude;
using namespace components;

enum class RenderGLStartupSets {
    context_creation,
    after_context_creation,
};

enum class RenderGLPipelineCompletionSets {
    pipeline_completion,
    after_pipeline_completion,
};

class RenderGLPlugin : public Plugin {
   public:
    void build(App& app) override;
};

/**
 * @brief Struct to query for a pipeline
 *
 * ### Usage
 *
 * ``for(auto [pipeline_layout, program, view_port, depth_range,
 * per_sample_operations, frame_buffer] : type<T>_instance.iter()) {}``
 */
struct PipelineQuery {
    /**
     * @brief Query for all pipelines
     *
     * @tparam Args Possible components to filter by
     */
    template <typename... Args>
    using query_type = Query<
        Get<PipelineLayout, ProgramPtr, ViewPort, DepthRange,
            PerSampleOperations, FrameBufferPtr>,
        With<Pipeline, Args...>, Without<>>;
    using bundle_type = std::tuple<
        PipelineLayout&, ProgramPtr&, ViewPort&, DepthRange&,
        PerSampleOperations&, FrameBufferPtr&>;
};
}  // namespace render_gl
}  // namespace pixel_engine
