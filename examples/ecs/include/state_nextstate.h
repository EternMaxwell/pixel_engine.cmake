#include <pixel_engine/entity.h>

namespace test_state {
    using namespace pixel_engine;

    int loops = 10;

    enum class State { Start = 0, Middle, End };

    void is_state_start(entity::Resource<entity::State<State>> state) {
        if (state.has_value()) {
            if (state.value().is_state(State::Start)) {
                std::cout << "is_state_start: true" << std::endl;
            } else {
                std::cout << "is_state_start: false" << std::endl;
            }
        }
    }

    void is_state_middle(entity::Resource<entity::State<State>> state) {
        if (state.has_value()) {
            if (state.value().is_state(State::Middle)) {
                std::cout << "is_state_middle: true" << std::endl;
            } else {
                std::cout << "is_state_middle: false" << std::endl;
            }
        }
    }

    void is_state_end(entity::Resource<entity::State<State>> state) {
        if (state.has_value()) {
            if (state.value().is_state(State::End)) {
                std::cout << "is_state_end: true" << std::endl;
            } else {
                std::cout << "is_state_end: false" << std::endl;
            }
        }
    }

    bool state_start(entity::Resource<entity::State<State>> state) {
        if (state.has_value()) {
            return state.value().is_state(State::Start);
        }
        return false;
    }

    bool state_middle(entity::Resource<entity::State<State>> state) {
        if (state.has_value()) {
            return state.value().is_state(State::Middle);
        }
        return false;
    }

    bool state_end(entity::Resource<entity::State<State>> state) {
        if (state.has_value()) {
            return state.value().is_state(State::End);
        }
        return false;
    }

    void set_state_start(entity::Resource<entity::NextState<State>> state) {
        if (state.has_value()) {
            state.value().set_state(State::Start);
        }
    }

    void set_state_middle(entity::Resource<entity::NextState<State>> state) {
        if (state.has_value()) {
            state.value().set_state(State::Middle);
        }
    }

    void set_state_end(entity::Resource<entity::NextState<State>> state) {
        if (state.has_value()) {
            state.value().set_state(State::End);
        }
    }

    void exit(entity::EventWriter<entity::AppExit> exit) {
        std::cout << "tick: " << loops << std::endl;
        if ((loops--) <= 0) exit.write(entity::AppExit{});
    }

    class StateTestPlugin : public entity::Plugin {
       public:
        void build(entity::App& app) override {
            entity::Node node1;
            app.init_state<State>()
                .add_plugin(entity::LoopPlugin{})
                .add_system(entity::Startup{}, is_state_start, &node1)
                .add_system(entity::OnEnter(State::Start), set_state_middle)
                .add_system(entity::OnEnter(State::Middle), set_state_end)
                .add_system(entity::Update{}, is_state_middle)
                .add_system(entity::Update{}, is_state_end)
                .add_system(entity::Update{}, exit);
        }
    };

    void test() {
        entity::App app;
        app.add_plugin(StateTestPlugin{}).run();
    }
}  // namespace test_state