#pragma once

#include <any>
#include <deque>
#include <memory>

namespace pixel_engine {
namespace app {
struct Event {
    uint32_t ticks = 0;
    std::any event;
};

template <typename Evt>
struct EventWriter {
   private:
    std::shared_ptr<std::deque<Event>> const m_events;

   public:
    EventWriter(std::shared_ptr<std::deque<Event>> events) : m_events(events) {}

    /*! @brief Write an event.
     * @param evt The event to be written.
     */
    auto& write(Evt evt) {
        m_events->push_back(Event{.event = evt});
        return *this;
    }
};

template <typename Evt>
struct EventReader {
   private:
    std::shared_ptr<std::deque<Event>> const m_events;

   public:
    EventReader(std::shared_ptr<std::deque<Event>> events) : m_events(events) {}

    class event_iter : public std::iterator<std::input_iterator_tag, Evt> {
       private:
        std::shared_ptr<std::deque<Event>> m_events;
        std::deque<Event>::iterator m_iter;

       public:
        event_iter(
            std::shared_ptr<std::deque<Event>> events,
            std::deque<Event>::iterator iter
        )
            : m_events(events), m_iter(iter) {}
        event_iter& operator++() {
            m_iter++;
            return *this;
        }
        event_iter operator++(int) {
            event_iter tmp = *this;
            ++(*this);
            return tmp;
        }
        bool operator==(const event_iter& rhs) const {
            return m_iter == rhs.m_iter;
        }
        bool operator!=(const event_iter& rhs) const {
            return m_iter != rhs.m_iter;
        }
        const Evt& operator*() { return std::any_cast<Evt&>((*m_iter).event); }
        event_iter begin() { return event_iter(m_events, m_events->begin()); }
        event_iter end() { return event_iter(m_events, m_events->end()); }
    };

    /*! @brief Get the iterator for the events.
     * @return The iterator for the events.
     */
    event_iter read() { return event_iter(m_events, m_events->begin()); }

    bool empty() { return m_events->empty(); }
};
}  // namespace app
}  // namespace pixel_engine