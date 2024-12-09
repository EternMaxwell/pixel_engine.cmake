#pragma once

#include <spdlog/spdlog.h>

#include "runner_tools.h"

namespace epix::app {
struct SubStageRunner {
    template <typename StageT>
    SubStageRunner(
        StageT stage, SubApp* src, SubApp* dst, WorkerPool* pools, SetMap* sets
    )
        : m_src(src),
          m_dst(dst),
          m_pools(pools),
          m_sets(sets),
          m_sub_stage(stage) {
        m_logger = spdlog::default_logger()->clone(
            std::string("sub_stage: ") + typeid(StageT).name() + "-" +
            std::to_string(static_cast<size_t>(stage))
        );
    }
    SubStageRunner(SubStageRunner&& other)            = default;
    SubStageRunner& operator=(SubStageRunner&& other) = default;
    template <typename StageT, typename... Args>
    SystemNode* add_system(StageT stage, void (*func)(Args...)) {
        if (auto it = m_systems.find(FuncIndex(func)); it != m_systems.end()) {
            m_logger->warn("System {:#018x} already present", (size_t)(func));
            return it->second.get();
        }
        auto ptr = std::make_shared<SystemNode>(stage, func);
        m_systems.emplace(FuncIndex(func), ptr);
        return ptr.get();
    }
    EPIX_API void build();
    EPIX_API void bake();
    EPIX_API void run(std::shared_ptr<SystemNode> node);
    EPIX_API void run();
    EPIX_API void set_log_level(spdlog::level::level_enum level);

   protected:
    SubApp* m_src;
    SubApp* m_dst;
    WorkerPool* m_pools;

    SystemStage m_sub_stage;
    MsgQueueBase<std::shared_ptr<SystemNode>> msg_queue;
    spp::sparse_hash_map<FuncIndex, std::shared_ptr<SystemNode>> m_systems;
    spp::sparse_hash_set<std::shared_ptr<SystemNode>> m_heads;
    SetMap* m_sets;

    std::shared_ptr<spdlog::logger> m_logger;
};
}  // namespace epix::app