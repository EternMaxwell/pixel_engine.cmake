#pragma once

#include <sparsepp/spp.h>

#include <BS_thread_pool.hpp>
#include <memory>
#include <mutex>
#include <queue>

#include "common.h"
#include "system.h"

namespace pixel_engine::app {
struct SystemStage {
    template <typename T>
    SystemStage(T stage)
        : m_stage(typeid(T)), m_sub_stage(static_cast<size_t>(stage)) {}
    EPIX_API bool operator==(const SystemStage& other) const;
    EPIX_API bool operator!=(const SystemStage& other) const;

    std::type_index m_stage;
    size_t m_sub_stage;
};
struct SystemSet {
    template <typename T>
    SystemSet(T set) : m_type(typeid(T)), m_value(static_cast<size_t>(set)) {}
    EPIX_API bool operator==(const SystemSet& other) const;
    EPIX_API bool operator!=(const SystemSet& other) const;

    std::type_index m_type;
    size_t m_value;
};
struct SetMap : spp::sparse_hash_map<std::type_index, std::vector<SystemSet>> {
};
struct SystemNode {
    template <typename StageT, typename... Args>
    SystemNode(StageT stage, void (*func)(Args...))
        : m_stage(stage), m_system(std::make_unique<System<Args...>>(func)) {
        m_sys_addr = (void*)func;
    }
    EPIX_API bool run(SubApp* src, SubApp* dst);
    EPIX_API void before(void* other_sys);
    EPIX_API void after(void* other_sys);
    EPIX_API void clear_tmp();
    EPIX_API double reach_time();

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
    EPIX_API BS::thread_pool* get_pool(const std::string& name);
    EPIX_API void add_pool(const std::string& name, uint32_t num_threads);

    spp::sparse_hash_map<std::string, std::unique_ptr<BS::thread_pool>> m_pools;
};
}  // namespace pixel_engine::app