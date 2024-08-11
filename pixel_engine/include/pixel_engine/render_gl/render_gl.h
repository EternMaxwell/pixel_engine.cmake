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

enum class RenderGLPreRenderSets {
    create_pipelines,
    after_create_pipelines,
};

class RenderGLPlugin : public entity::Plugin {
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
        Get<pipeline::PipelineLayout, pipeline::ProgramPtr, pipeline::ViewPort,
            pipeline::DepthRange, pipeline::PerSampleOperations,
            pipeline::FrameBufferPtr>,
        With<pipeline::Pipeline, Args...>, Without<>>;
    using bundle_type = std::tuple<
        pipeline::PipelineLayout&, pipeline::ProgramPtr&, pipeline::ViewPort&,
        pipeline::DepthRange&, pipeline::PerSampleOperations&,
        pipeline::FrameBufferPtr&>;
};
}  // namespace render_gl
}  // namespace pixel_engine
