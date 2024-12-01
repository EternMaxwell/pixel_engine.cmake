#pragma once

#include <sparsepp/spp.h>

#include "substage_runner.h"

namespace pixel_engine::app {
struct StageRunner {
    EPIX_API StageRunner(
        std::type_index stage,
        SubApp* src,
        SubApp* dst,
        WorkerPool* pools,
        SetMap* sets
    );
    EPIX_API StageRunner(StageRunner&& other)   = default;
    EPIX_API StageRunner& operator=(StageRunner&& other) = default;

    template <typename StageT>
    void add_sub_stage(StageT sub_stage) {
        if (std::type_index(typeid(StageT)) != m_stage) {
            spdlog::warn(
                "Stage {} cannot add sub stage {} - {}", m_stage.name(),
                typeid(StageT).name(), static_cast<size_t>(sub_stage)
            );
            return;
        }
        m_sub_stages.emplace(
            static_cast<size_t>(sub_stage),
            std::make_unique<SubStageRunner>(
                sub_stage, m_src, m_dst, m_pools, m_sets
            )
        );
    }

    template <typename StageT, typename... Subs>
    void configure_sub_stage(StageT sub_stage, Subs... sub_stages) {
        add_sub_stage(sub_stage);
        (add_sub_stage(sub_stages), ...);
        m_sub_stage_order = {
            static_cast<size_t>(sub_stage), static_cast<size_t>(sub_stages)...
        };
    }

    template <typename StageT, typename... Args>
    SystemNode* add_system(StageT stage, void (*func)(Args...)) {
        if (auto it = m_sub_stages.find(static_cast<size_t>(stage));
            it == m_sub_stages.end()) {
            m_sub_stages.emplace(
                static_cast<size_t>(stage),
                std::make_unique<SubStageRunner>(
                    stage, m_src, m_dst, m_pools, m_sets
                )
            );
        }
        return m_sub_stages[static_cast<size_t>(stage)]->add_system(
            stage, func
        );
    }

    EPIX_API bool conflict(const StageRunner* other) const;

    EPIX_API void build();
    EPIX_API void bake();
    EPIX_API void run();
    EPIX_API void set_log_level(spdlog::level::level_enum level);

   protected:
    SubApp* m_src;
    SubApp* m_dst;
    WorkerPool* m_pools;
    SetMap* m_sets;

    std::type_index m_stage;
    spp::sparse_hash_map<size_t, std::unique_ptr<SubStageRunner>> m_sub_stages;
    std::vector<size_t> m_sub_stage_order;
    std::shared_ptr<spdlog::logger> m_logger;
};
}  // namespace pixel_engine::app