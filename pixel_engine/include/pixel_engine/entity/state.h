#pragma once

namespace pixel_engine {
    namespace entity {
        template <typename T, std::enable_if<std::is_enum_v<T>>::type* = nullptr>
        class State {
           protected:
            T m_state;

           public:
            State() : m_state() {}
            State(const T& state) : m_state(state) {}

            bool is_state(const T& state) { return m_state == state; }
        };

        template <typename T>
        class NextState : public State<T> {
           public:
            NextState() : State<T>() {}
            NextState(const T& state) : State<T>(state) {}

            void set_state(const T& state) { State<T>::m_state = state; }
        };
    }  // namespace entity
}  // namespace pixel_engine