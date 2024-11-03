#pragma once

#include <deque>

namespace pixel_engine::app {
struct EventQueueBase {
    virtual void tick() = 0;
};

template <typename T>
struct EventQueue : public EventQueueBase {
    void tick() override {
        for (auto it = m_queue.begin(); it != m_queue.end();) {
            if (it->second == 0) {
                m_queue.erase(it);
            } else {
                it->second--;
                it++;
            }
        }
    }
    void push(const T& event) { m_queue.emplace_back(event, 1); }
    void push(T&& event) { m_queue.emplace_back(event, 1); }
    struct iterator {
        std::deque<std::pair<T, uint8_t>>::iterator it;
        iterator operator++() {
            it++;
            return *this;
        }
        bool operator!=(const iterator& other) { return it != other.it; }
        const T& operator*() { return it->first; }
    };
    iterator begin() { return iterator{m_queue.begin()}; }
    iterator end() { return iterator{m_queue.end()}; }

   private:
    std::deque<std::pair<T, uint8_t>> m_queue;
};
}  // namespace pixel_engine::app