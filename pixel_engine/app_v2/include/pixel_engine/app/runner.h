#pragma once

#include "stage_runner.h"
#include "subapp.h"

namespace pixel_engine::app {
struct Runner {
    Runner(spp::sparse_hash_map<std::type_index, std::unique_ptr<SubApp>>*
               sub_apps);
    Runner(Runner&& other)            = default;
    Runner& operator=(Runner&& other) = default;

    struct StageNode {
        std::type_index stage;
        std::unique_ptr<StageRunner> runner;
        spp::sparse_hash_set<std::weak_ptr<StageNode>> strong_prev_stages;
        spp::sparse_hash_set<std::weak_ptr<StageNode>> strong_next_stages;
        spp::sparse_hash_set<std::weak_ptr<StageNode>> weak_prev_stages;
        spp::sparse_hash_set<std::weak_ptr<StageNode>> weak_next_stages;
        spp::sparse_hash_set<std::type_index> prev_stages;
        spp::sparse_hash_set<std::type_index> next_stages;
        size_t prev_count;
        std::optional<size_t> depth;
        StageNode(std::type_index stage, std::unique_ptr<StageRunner>&& runner);
        template <typename T>
        void add_prev_stage() {
            prev_stages.insert(std::type_index(typeid(T)));
        }
        template <typename T>
        void add_next_stage() {
            next_stages.insert(std::type_index(typeid(T)));
        }
        void clear_tmp();
        size_t get_depth();
    };

    template <typename SrcT, typename DstT, typename StageT, typename... Subs>
    StageNode* assign_startup_stage(StageT stage, Subs... sub_stages) {
        auto& src   = m_sub_apps->at(std::type_index(typeid(SrcT)));
        auto& dst   = m_sub_apps->at(std::type_index(typeid(DstT)));
        auto runner = std::make_unique<StageRunner>(
            std::type_index(typeid(StageT)), src.get(), dst.get(),
            m_pools.get(), m_sets.get()
        );
        runner->configure_sub_stage(stage, sub_stages...);
        auto node = std::make_shared<StageNode>(
            std::type_index(typeid(StageT)), std::move(runner)
        );
        m_startup_stages.emplace(std::type_index(typeid(StageT)), node);
        return node.get();
    }
    template <typename SrcT, typename DstT, typename StageT, typename... Subs>
    StageNode* assign_loop_stage(StageT stage, Subs... sub_stages) {
        auto& src   = m_sub_apps->at(std::type_index(typeid(SrcT)));
        auto& dst   = m_sub_apps->at(std::type_index(typeid(DstT)));
        auto runner = std::make_unique<StageRunner>(
            std::type_index(typeid(StageT)), src.get(), dst.get(),
            m_pools.get(), m_sets.get()
        );
        runner->configure_sub_stage(stage, sub_stages...);
        auto node = std::make_shared<StageNode>(
            std::type_index(typeid(StageT)), std::move(runner)
        );
        m_loop_stages.emplace(std::type_index(typeid(StageT)), node);
        return node.get();
    }
    template <typename SrcT, typename DstT, typename StageT, typename... Subs>
    StageNode* assign_state_transition_stage(StageT stage, Subs... sub_stages) {
        auto& src   = m_sub_apps->at(std::type_index(typeid(SrcT)));
        auto& dst   = m_sub_apps->at(std::type_index(typeid(DstT)));
        auto runner = std::make_unique<StageRunner>(
            std::type_index(typeid(StageT)), src.get(), dst.get(),
            m_pools.get(), m_sets.get()
        );
        runner->configure_sub_stage(stage, sub_stages...);
        auto node = std::make_shared<StageNode>(
            std::type_index(typeid(StageT)), std::move(runner)
        );
        m_state_transition_stages.emplace(
            std::type_index(typeid(StageT)), node
        );
        return node.get();
    }
    template <typename SrcT, typename DstT, typename StageT, typename... Subs>
    StageNode* assign_exit_stage(StageT stage, Subs... sub_stages) {
        auto& src   = m_sub_apps->at(std::type_index(typeid(SrcT)));
        auto& dst   = m_sub_apps->at(std::type_index(typeid(DstT)));
        auto runner = std::make_unique<StageRunner>(
            std::type_index(typeid(StageT)), src.get(), dst.get(),
            m_pools.get(), m_sets.get()
        );
        runner->configure_sub_stage(stage, sub_stages...);
        auto node = std::make_shared<StageNode>(
            std::type_index(typeid(StageT)), std::move(runner)
        );
        m_exit_stages.emplace(std::type_index(typeid(StageT)), node);
        return node.get();
    }

    template <typename StageT>
    bool stage_startup() {
        return m_startup_stages.find(std::type_index(typeid(StageT))) !=
               m_startup_stages.end();
    }
    bool stage_startup(std::type_index stage);
    template <typename StageT>
    bool stage_loop() {
        return m_loop_stages.find(std::type_index(typeid(StageT))) !=
               m_loop_stages.end();
    }
    bool stage_loop(std::type_index stage);
    template <typename StageT>
    bool stage_state_transition() {
        return m_state_transition_stages.find(std::type_index(typeid(StageT))
               ) != m_state_transition_stages.end();
    }
    bool stage_state_transition(std::type_index stage);
    template <typename StageT>
    bool stage_exit() {
        return m_exit_stages.find(std::type_index(typeid(StageT))) !=
               m_exit_stages.end();
    }
    bool stage_exit(std::type_index stage);

    template <typename SetT, typename... Sets>
    void configure_sets(SetT set, Sets... sets) {
        m_sets->emplace(
            std::type_index(typeid(SetT)), std::vector<SystemSet>{set, sets...}
        );
    }

    template <typename StageT, typename... Args>
    SystemNode* add_system(StageT stage, void (*func)(Args...)) {
        if (auto it = m_startup_stages.find(std::type_index(typeid(StageT)));
            it != m_startup_stages.end()) {
            return it->second->runner->add_system(stage, func);
        }
        if (auto it = m_loop_stages.find(std::type_index(typeid(StageT)));
            it != m_loop_stages.end()) {
            return it->second->runner->add_system(stage, func);
        }
        if (auto it =
                m_state_transition_stages.find(std::type_index(typeid(StageT)));
            it != m_state_transition_stages.end()) {
            return it->second->runner->add_system(stage, func);
        }
        if (auto it = m_exit_stages.find(std::type_index(typeid(StageT)));
            it != m_exit_stages.end()) {
            return it->second->runner->add_system(stage, func);
        }
        spdlog::warn("Stage {} not found", typeid(StageT).name());
        return nullptr;
    }

    void build();
    void bake_all();
    void run(std::shared_ptr<StageNode> node);
    void run_startup();
    void run_loop();
    void run_state_transition();
    void run_exit();
    void tick_events();
    void end_commands();
    void update_states();
    void add_worker(const std::string& name, uint32_t num_threads);
    void set_log_level(spdlog::level::level_enum level);

   protected:
    MsgQueueBase<std::shared_ptr<StageNode>> msg_queue;
    spp::sparse_hash_map<std::type_index, std::unique_ptr<SubApp>>* m_sub_apps;
    spp::sparse_hash_map<std::type_index, std::shared_ptr<StageNode>>
        m_startup_stages;
    spp::sparse_hash_map<std::type_index, std::shared_ptr<StageNode>>
        m_loop_stages;
    spp::sparse_hash_map<std::type_index, std::shared_ptr<StageNode>>
        m_state_transition_stages;
    spp::sparse_hash_map<std::type_index, std::shared_ptr<StageNode>>
        m_exit_stages;
    std::unique_ptr<WorkerPool> m_pools;
    std::unique_ptr<BS::thread_pool> m_control_pool;
    std::unique_ptr<SetMap> m_sets;

    std::shared_ptr<spdlog::logger> m_logger;
};
}  // namespace pixel_engine::app