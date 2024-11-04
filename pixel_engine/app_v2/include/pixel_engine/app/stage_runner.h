#pragma once

#include <sparsepp/spp.h>

#include "substage_runner.h"

namespace pixel_engine::app {
struct StageRunner {
    StageRunner(
        const type_info* stage,
        SubApp* src,
        SubApp* dst,
        WorkerPool* pools,
        SetMap* sets
    )
        : m_src(src),
          m_dst(dst),
          m_pools(pools),
          m_sets(sets),
          m_stage(stage) {}
    StageRunner(StageRunner&& other)            = default;
    StageRunner& operator=(StageRunner&& other) = default;

    template <typename StageT>
    void add_sub_stage(StageT sub_stage) {
        if (typeid(StageT) != *m_stage) {
            spdlog::warn(
                "Stage {} cannot add sub stage {} - {}", m_stage->name(),
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

    void build() {
        for (auto& [sub_stage, sub_stage_runner] : m_sub_stages) {
            sub_stage_runner->build();
        }
    }

    void bake() {
        for (auto& [sub_stage, sub_stage_runner] : m_sub_stages) {
            sub_stage_runner->bake();
        }
    }

    void run() {
        for (auto sub_stage : m_sub_stage_order) {
            m_sub_stages[sub_stage]->run();
        }
    }

    SubApp* m_src;
    SubApp* m_dst;
    WorkerPool* m_pools;
    SetMap* m_sets;

    const type_info* m_stage;
    spp::sparse_hash_map<size_t, std::unique_ptr<SubStageRunner>> m_sub_stages;
    std::vector<size_t> m_sub_stage_order;
};
}  // namespace pixel_engine::app