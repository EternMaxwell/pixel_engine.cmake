#include "pixel_engine/app/runner_tools.h"

using namespace pixel_engine::app;

bool SystemStage::operator==(const SystemStage& other) const {
    return m_stage == other.m_stage && m_sub_stage == other.m_sub_stage;
}
bool SystemStage::operator!=(const SystemStage& other) const {
    return m_stage != other.m_stage && m_sub_stage != other.m_sub_stage;
}

bool SystemSet::operator==(const SystemSet& other) const {
    return m_type == other.m_type && m_value == other.m_value;
}
bool SystemSet::operator!=(const SystemSet& other) const {
    return m_type != other.m_type && m_value != other.m_value;
}

bool SystemNode::run(SubApp* src, SubApp* dst) {
    for (auto& cond : m_conditions) {
        if (!cond->run(src, dst)) return false;
    }
    m_system->run(src, dst);
    return true;
}
void SystemNode::before(void* other_sys) { m_ptr_nexts.insert(other_sys); }
void SystemNode::after(void* other_sys) { m_ptr_prevs.insert(other_sys); }
void SystemNode::clear_tmp() {
    m_weak_nexts.clear();
    m_weak_prevs.clear();
    m_reach_time.reset();
}
double SystemNode::reach_time() {
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

BS::thread_pool* WorkerPool::get_pool(const std::string& name) {
    auto it = m_pools.find(name);
    if (it == m_pools.end()) {
        return nullptr;
    }
    return m_pools[name].get();
}
void WorkerPool::add_pool(const std::string& name, uint32_t num_threads) {
    m_pools.emplace(name, std::make_unique<BS::thread_pool>(num_threads));
}