#pragma once

#include "event_queue.h"

namespace pixel_engine::app {
template <typename T>
struct EventReader {
    EventReader(EventQueueBase* queue) : m_queue(queue) {}

    struct iter {
        iter(EventQueue<T>* queue) : m_queue(queue) {}
        auto begin() { return m_queue->begin(); }
        auto end() { return m_queue->end(); }

       private:
        EventQueue<T>* m_queue;
    };

    iter read() { return iter(m_queue); }

   private:
    EventQueue<T>* m_queue;
};

template <typename T>
struct EventWriter {
    EventWriter(EventQueueBase* queue) : m_queue(queue) {}

    void write(const T& event) { m_queue->push(event); }
    void write(T&& event) { m_queue->push(std::move(event)); }

   private:
    EventQueue<T>* m_queue;
};
}  // namespace pixel_engine::app