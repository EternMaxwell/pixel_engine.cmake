#pragma once

namespace pixel_engine {
    namespace entity {
        class App;
        template <typename T>
        class NextState;

        template <typename T,
                  std::enable_if<std::is_enum_v<T>>::type* = nullptr>
        class State {
            friend class App;

           protected:
            T m_state;

           public:
            State() : m_state() {}
            State(const T& state) : m_state(state) {}

            bool is_state(const T& state) { return m_state == state; }
            bool is_state(const NextState<T>& state) {
                return m_state == state.m_state;
            }
        };

        template <typename T>
        class NextState : public State<T> {
            friend class App;

           protected:
            bool has_next = false;
            bool applied = false;
            void apply() {
                applied = true;
                has_next = false;
            }
            void reset() { applied = false; }
            bool has_next_state() { return has_next; }

           public:
            NextState() : State<T>() {}
            NextState(const T& state) : State<T>(state) {}

            void set_state(const T& state) {
                State<T>::m_state = state;
                has_next = true;
            }
        };
    }  // namespace entity
}  // namespace pixel_engine