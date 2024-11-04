#pragma once

#include <sparsepp/spp.h>

#include <BS_thread_pool.hpp>
#include <memory>
#include <mutex>
#include <queue>

#include "system.h"

namespace pixel_engine::app {
struct SystemStage {
    SystemStage() = default;
    template <typename T>
    SystemStage(T stage)
        : m_stage(&typeid(T)), m_sub_stage(static_cast<size_t>(stage)) {}
    bool operator==(const SystemStage& other) {
        return m_stage == other.m_stage && m_sub_stage == other.m_sub_stage;
    }
    bool operator!=(const SystemStage& other) {
        return m_stage != other.m_stage && m_sub_stage != other.m_sub_stage;
    }

    const type_info* m_stage;
    size_t m_sub_stage;
};
struct SystemSet {
    template <typename T>
    SystemSet(T set) : m_type(&typeid(T)), m_value(static_cast<size_t>(set)) {}
    bool operator==(const SystemSet& other) {
        return m_type == other.m_type && m_value == other.m_value;
    }
    bool operator!=(const SystemSet& other) {
        return m_type != other.m_type && m_value != other.m_value;
    }

    const type_info* m_type;
    size_t m_value;
};
struct SetMap : spp::sparse_hash_map<const type_info*, std::vector<SystemSet>> {
};
struct SystemNode {
    SystemNode() = default;
    template <typename StageT, typename... Args>
    SystemNode(StageT stage, void (*func)(Args...))
        : m_stage(stage), m_system(std::make_unique<System<Args...>>(func)) {
        m_sys_addr = (void*)func;
    }
    bool run(SubApp* src, SubApp* dst) {
        for (auto& cond : m_conditions) {
            if (!cond->run(src, dst)) return false;
        }
        m_system->run(src, dst);
        return true;
    }
    void before(void* other_sys) { m_ptr_nexts.insert(other_sys); }
    void after(void* other_sys) { m_ptr_prevs.insert(other_sys); }
    void clear_tmp() {
        m_weak_nexts.clear();
        m_weak_prevs.clear();
        m_reach_time.reset();
    }
    double reach_time() {
        if (m_reach_time) return m_reach_time.value();
        m_reach_time = 0;
        for (auto& bef : m_strong_prevs) {
            if (auto bef_sys = bef.lock()) {
                m_reach_time = std::max(
                    m_reach_time.value(),
                    bef_sys->reach_time() + bef_sys->m_system->get_avg_time()
                );
            }
        }
        return m_reach_time.value();
    }

    SystemStage m_stage;
    std::vector<SystemSet> m_in_sets;
    void* m_sys_addr;
    std::unique_ptr<BasicSystem<void>> m_system;
    std::string m_worker = "default";
    spp::sparse_hash_set<std::weak_ptr<SystemNode>> m_strong_prevs;
    spp::sparse_hash_set<std::weak_ptr<SystemNode>> m_strong_nexts;
    spp::sparse_hash_set<std::weak_ptr<SystemNode>> m_weak_prevs;
    spp::sparse_hash_set<std::weak_ptr<SystemNode>> m_weak_nexts;
    spp::sparse_hash_set<void*> m_ptr_prevs;
    spp::sparse_hash_set<void*> m_ptr_nexts;
    std::vector<std::unique_ptr<BasicSystem<bool>>> m_conditions;

    std::optional<double> m_reach_time;
    size_t m_prev_count;
    size_t m_next_count;
};
template <typename T>
struct MsgQueueBase {
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    MsgQueueBase() = default;
    MsgQueueBase(MsgQueueBase&& other) : m_queue(std::move(other.m_queue)) {}
    auto& operator=(MsgQueueBase&& other) {
        m_queue = std::move(other.m_queue);
        return *this;
    }
    void push(T elem) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(elem);
        m_cv.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        T system = m_queue.front();
        m_queue.pop();
        return system;
    }
    T front() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.front();
    }
    bool empty() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    void wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_queue.empty(); });
    }
};
struct WorkerPool {
    spp::sparse_hash_map<std::string, std::unique_ptr<BS::thread_pool>> m_pools;
    BS::thread_pool* get_pool(const std::string& name) {
        auto it = m_pools.find(name);
        if (it == m_pools.end()) {
            return nullptr;
        }
        return m_pools[name].get();
    }
    void add_pool(const std::string& name, uint32_t num_threads) {
        m_pools.emplace(name, std::make_unique<BS::thread_pool>(num_threads));
    }
};
}  // namespace pixel_engine::app