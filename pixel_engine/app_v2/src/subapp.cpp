#include "pixel_engine/app/subapp.h"

using namespace pixel_engine::app;

void SubApp::tick_events() {
    for (auto& [ptr, queue] : m_world.m_event_queues) {
        queue->tick();
    }
}

void SubApp::end_commands() {
    for (auto& command : m_command_cache) {
        command.end();
    }
    m_command_cache.clear();
}

void SubApp::update_states() {
    for (auto& update : m_state_updates) {
        update->run(this, this);
    }
}