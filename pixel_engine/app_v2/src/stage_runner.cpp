#include "pixel_engine/app/stage_runner.h"

using namespace pixel_engine::app;

StageRunner::StageRunner(
    const type_info* stage,
    SubApp* src,
    SubApp* dst,
    WorkerPool* pools,
    SetMap* sets
)
    : m_src(src), m_dst(dst), m_pools(pools), m_sets(sets), m_stage(stage) {}

void StageRunner::build() {
    for (auto& [sub_stage, sub_stage_runner] : m_sub_stages) {
        sub_stage_runner->build();
    }
}

void StageRunner::bake() {
    for (auto& [sub_stage, sub_stage_runner] : m_sub_stages) {
        sub_stage_runner->bake();
    }
}

void StageRunner::run() {
    for (auto sub_stage : m_sub_stage_order) {
        m_sub_stages[sub_stage]->run();
    }
}