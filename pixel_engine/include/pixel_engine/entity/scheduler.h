﻿#pragma once

#include "pixel_engine/entity/resource.h"
#include "pixel_engine/entity/state.h"

namespace pixel_engine {
    namespace entity {
        class App;

        class Scheduler {
           public:
            virtual bool should_run(App* app) = 0;
        };

        class Startup : public Scheduler {
           private:
            bool m_value;

           public:
            Startup() : m_value(true), Scheduler() {}
            bool should_run(App* app) override {
                if (m_value) {
                    m_value = false;
                    return true;
                } else {
                    return false;
                }
            }
        };

        class Update : public Scheduler {
           public:
            Update() : Scheduler() {}
            bool should_run(App* app) override { return true; }
        };

        class Render : public Scheduler {
           public:
            Render() : Scheduler() {}
            bool should_run(App* app) override { return true; }
        };

        class OnStateChange : public Scheduler {};

        template <typename T>
        class OnEnter : public OnStateChange {
           private:
            T m_state;
            App* m_app;

           public:
            OnEnter(T state) : m_state(state) {}
            bool should_run(App* app) override {
                m_app = app;
                return m_app->run_system_v(
                    std::function([&](Resource<NextState<T>> state_next, Resource<State<T>> state) {
                        if (state.has_value()) {
                            if (state.value().is_state(m_state)) {
                                if (state.value().is_just_created()) {
                                    return true;
                                } else if (state_next.has_value()) {
                                    if (!state_next.value().is_state(m_state)) {
                                        return true;
                                    }
                                }
                            }
                        }
                        return false;
                    }));
            }
        };

        template <typename T>
        class OnExit : public Scheduler {
           private:
            T m_state;
            App* m_app;

           public:
            OnExit(T state) : m_state(state) {}
            bool should_run(App* app) override {
                m_app = app;
                return m_app->run_system_v(
                    std::function([&](Resource<NextState<T>> state_next, Resource<State<T>> state) {
                        if (state.has_value()) {
                            if (state.value().is_state(m_state)) {
                                if (state_next.has_value()) {
                                    if (!state_next.value().is_state(m_state)) {
                                        return true;
                                    }
                                }
                            }
                        }
                        return false;
                    }));
            }
        };
    }  // namespace entity
}  // namespace pixel_engine