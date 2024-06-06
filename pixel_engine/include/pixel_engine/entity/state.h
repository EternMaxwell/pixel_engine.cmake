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
            bool just_created = true;

           public:
            State() : m_state() {}
            State(const T& state) : m_state(state) {}
            bool is_just_created() { return just_created; }

            bool is_state(const T& state) { return m_state == state; }
            bool is_state(const NextState<T>& state) {
                return m_state == state.m_state;
            }
        };

        template <typename T>
        class NextState : public State<T> {
            friend class App;

           public:
            NextState() : State<T>() {}
            NextState(const T& state) : State<T>(state) {}

            void set_state(const T& state) {
                State<T>::m_state = state;
            }
        };
    }  // namespace entity
}  // namespace pixel_engine