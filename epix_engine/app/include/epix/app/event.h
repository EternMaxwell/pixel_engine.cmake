#pragma once

#include "event_queue.h"

namespace epix::app {
template <typename T>
struct EventReader {
    EventReader(EventQueueBase* queue)
        : m_queue(dynamic_cast<EventQueue<T>*>(queue)) {}

    struct iter {
        iter(EventQueue<T>* queue) : m_queue(queue) {}
        auto begin() { return m_queue->begin(); }
        auto end() { return m_queue->end(); }

       private:
        EventQueue<T>* m_queue;
    };

    iter read() { return iter(m_queue); }
    void clear() { m_queue->clear(); }
    bool empty() { return m_queue->empty(); }

   private:
    EventQueue<T>* m_queue;
};

template <typename T>
struct EventWriter {
    EventWriter(EventQueueBase* queue)
        : m_queue(dynamic_cast<EventQueue<T>*>(queue)) {}

    EventWriter& write(const T& event) {
        m_queue->push(event);
        return *this;
    }
    EventWriter& write(T&& event) {
        m_queue->push(std::forward<T>(event));
        return *this;
    }

   private:
    EventQueue<T>* m_queue;
};
}  // namespace epix::app