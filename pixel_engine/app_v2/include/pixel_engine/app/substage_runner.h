#pragma once

#include <spdlog/spdlog.h>

#include "runner_tools.h"

namespace pixel_engine::app {
struct SubStageRunner {
    template <typename StageT>
    SubStageRunner(
        StageT stage, SubApp* src, SubApp* dst, WorkerPool* pools, SetMap* sets
    )
        : m_src(src),
          m_dst(dst),
          m_pools(pools),
          m_sets(sets),
          m_sub_stage(stage) {}

    SubStageRunner(SubStageRunner&& other)            = default;
    SubStageRunner& operator=(SubStageRunner&& other) = default;

    SubApp* m_src;
    SubApp* m_dst;
    WorkerPool* m_pools;

    SystemStage m_sub_stage;
    MsgQueueBase<std::shared_ptr<SystemNode>> msg_queue;
    spp::sparse_hash_map<void*, std::shared_ptr<SystemNode>> m_systems;
    std::shared_ptr<SystemNode> m_head;
    SetMap* m_sets;
    template <typename StageT, typename... Args>
    SystemNode* add_system(StageT stage, void (*func)(Args...)) {
        auto ptr = std::make_shared<SystemNode>(stage, func);
        m_systems.emplace((void*)(func), ptr);
        return ptr.get();
    }
    void build();
    void bake();
    void run(std::shared_ptr<SystemNode> node);
    void run();
};
}  // namespace pixel_engine::app