﻿#include <pixel_engine/entity.h>

namespace test_state {
    using namespace pixel_engine;
    using namespace prelude;

    int loops = 10;

    enum class States { Start = 0, Middle, End };

    void is_state_start(Resource<State<States>> state) {
        if (state.has_value()) {
            if (state->is_state(States::Start)) {
                std::cout << "is_state_start: true" << std::endl;
            } else {
                std::cout << "is_state_start: false" << std::endl;
            }
        }
    }

    void is_state_middle(Resource<State<States>> state) {
        if (state.has_value()) {
            if (state->is_state(States::Middle)) {
                std::cout << "is_state_middle: true" << std::endl;
            } else {
                std::cout << "is_state_middle: false" << std::endl;
            }
        }
    }

    void is_state_end(Resource<State<States>> state) {
        if (state.has_value()) {
            if (state->is_state(States::End)) {
                std::cout << "is_state_end: true" << std::endl;
            } else {
                std::cout << "is_state_end: false" << std::endl;
            }
        }
    }

    void set_state_start(Resource<NextState<States>> state) {
        if (state.has_value()) {
            state->set_state(States::Start);
        }
    }

    void set_state_middle(Resource<NextState<States>> state) {
        if (state.has_value()) {
            state->set_state(States::Middle);
        }
    }

    void set_state_end(Resource<NextState<States>> state) {
        if (state.has_value()) {
            state->set_state(States::End);
        }
    }

    void exit(EventWriter<AppExit> exit) {
        std::cout << "tick: " << loops << std::endl;
        if ((loops--) <= 0) exit.write(AppExit{});
    }

    class StateTestPlugin : public Plugin {
       public:
        void build(App& app) override {
            SystemNode node;
            app.init_state<States>()
                .add_system(Startup{}, is_state_start)
                .add_system(OnEnter(States::Start), set_state_middle)
                .add_system(OnEnter(States::Middle), set_state_end)
                .add_system(Update{}, is_state_middle, &node)
                .add_system(Update{}, is_state_end, &node, after(node))
                .add_system(Update{}, exit, &node, after(node));
        }
    };

    void test() {
        App app;
        app.add_plugin(StateTestPlugin{}).add_plugin(LoopPlugin{}).run();
    }
}  // namespace test_state