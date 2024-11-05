#pragma once

#include <deque>

namespace pixel_engine::app {
struct EventQueueBase {
    virtual void tick() = 0;
};

template <typename T>
struct EventQueue : public EventQueueBase {
    struct elem {
        T event;
        uint8_t count;
    };
    void tick() override {
        while (!m_queue.empty()) {
            auto& front = m_queue.front();
            if (front.count == 0) {
                m_queue.pop_front();
            } else {
                break;
            }
        }
        for (auto it = m_queue.begin(); it != m_queue.end();) {
            it->count--;
            it++;
        }
    }
    void push(const T& event) { m_queue.push_back({event, 1}); }
    void push(T&& event) { m_queue.push_back({std::forward<T>(event), 1}); }
    void clear() { m_queue.clear(); }
    bool empty() const { return m_queue.empty(); }
    struct iterator {
        std::deque<elem>::iterator it;
        iterator operator++() {
            it++;
            return *this;
        }
        bool operator!=(const iterator& other) { return it != other.it; }
        const T& operator*() { return it->event; }
    };
    iterator begin() { return iterator{m_queue.begin()}; }
    iterator end() { return iterator{m_queue.end()}; }

   private:
    std::deque<elem> m_queue;
};
}  // namespace pixel_engine::app