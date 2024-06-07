#pragma once

#include <any>
#include <deque>
#include <entt/entity/registry.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <pfr.hpp>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_map>

namespace pixel_engine {
    namespace entity {
        struct Event {
            uint32_t ticks = 0;
            std::any event;
        };

        template <typename Evt>
        class EventWriter {
           private:
            std::deque<Event>* const m_events;

           public:
            EventWriter(std::deque<Event>* events) : m_events(events) {}

            /*! @brief Write an event.
             * @param evt The event to be written.
             */
            auto& write(Evt evt) {
                m_events->push_back(Event{.event = evt});
                return *this;
            }
        };

        template <typename Evt>
        class EventReader {
           private:
            std::deque<Event>* const m_events;

           public:
            EventReader(std::deque<Event>* events) : m_events(events) {}

            class event_iter : public std::iterator<std::input_iterator_tag, Evt> {
               private:
                std::deque<Event>* m_events;
                std::deque<Event>::iterator m_iter;

               public:
                event_iter(std::deque<Event>* events, std::deque<Event>::iterator iter)
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
                bool operator==(const event_iter& rhs) const { return m_iter == rhs.m_iter; }
                bool operator!=(const event_iter& rhs) const { return m_iter != rhs.m_iter; }
                Evt& operator*() { return std::any_cast<Evt&>((*m_iter).event); }

                event_iter begin() { return event_iter(m_events, m_events->begin()); }

                event_iter end() { return event_iter(m_events, m_events->end()); }
            };

            /*! @brief Read an event.
             * @return The iterator for the events.
             */
            auto read() { return event_iter(m_events, m_events->begin()); }

            /*! @brief Check if the event queue is empty.
             * @return True if the event queue is empty, false otherwise.
             */
            bool empty() { return m_events->empty(); }

            /*! @brief Clear the event queue.
             */
            void clear() { m_events->clear(); }
        };
    }  // namespace entity
}  // namespace pixel_engine