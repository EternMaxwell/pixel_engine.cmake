#pragma once

#include "runner.h"

namespace pixel_engine::prelude {
enum MainStartupStage {
    PreStartup,
    Startup,
    PostStartup,
};
enum MainLoopStage {
    First,
    PreUpdate,
    Update,
    PostUpdate,
    Last,
};
enum MainExitStage {
    PreExit,
    Exit,
    PostExit,
};
enum RenderStartupStage {
    PreRenderStartup,
    RenderStartup,
    PostRenderStartup,
};
enum RenderLoopStage {
    PreRender,
    Render,
    PostRender,
};
enum RenderExitStage {
    PreRenderExit,
    RenderExit,
    PostRenderExit,
};
}  // namespace pixel_engine::prelude
namespace pixel_engine::app {
using namespace pixel_engine::prelude;
struct AppExit {};
struct App;
struct MainSubApp {};
struct RenderSubApp {};
struct Plugin {
    virtual void build(App&) = 0;
};
struct App {
    static App create() {
        App app;
        app.add_sub_app<RenderSubApp>();
        app.m_runner->assign_startup_stage<MainSubApp, MainSubApp>(
            PreStartup, Startup, PostStartup
        );
        app.m_runner->assign_loop_stage<MainSubApp, MainSubApp>(
            First, PreUpdate, Update, PostUpdate, Last
        );
        app.m_runner
            ->assign_loop_stage<MainSubApp, MainSubApp>(
                PreRender, Render, PostRender
            )
            ->add_prev_stage<MainLoopStage>();
        // this is temporary, currently the render stages are still in main
        // subapp
        app.m_runner->assign_exit_stage<MainSubApp, MainSubApp>(
            PreExit, Exit, PostExit
        );
        return std::move(app);
    }
    struct SystemInfo {
        SystemNode* node;
        App* app;

        template <typename... Ptrs>
        SystemInfo& before(Ptrs... ptrs) {
            (node->before(ptrs), ...);
            return *this;
        }
        template <typename... Ptrs>
        SystemInfo& after(Ptrs... ptrs) {
            (node->after(ptrs), ...);
            return *this;
        }
        template <typename... Args>
        SystemInfo& run_if(std::function<bool(Args...)> func) {
            node->m_conditions.emplace_back(
                std::make_unique<Condition<Args...>>(func)
            );
            return *this;
        }
        template <typename... Args>
        SystemInfo& use_worker(const std::string& pool_name) {
            node->m_worker = pool_name;
            return *this;
        }
        template <typename... Sets>
        SystemInfo& in_sets(Sets... sets) {
            (node->m_in_sets.insert(SystemSet(sets)), ...);
            return *this;
        }
        template <typename T>
        SystemInfo& on_enter(T state) {
            if (app->m_runner->stage_state_transition(node->m_stage.m_stage)) {
                node->m_conditions.emplace_back(
                    std::make_unique<Condition<State<T>, NextState<T>>>(
                        [state](State<T> s, NextState<T> ns) {
                            return (s.is_state(state) && s.is_just_created()) ||
                                   (ns.is_state(state) && !s.is_state(state));
                        }
                    )
                );
            } else {
                spdlog::warn(
                    "adding system {:#018x} to stage {} is not allowed"
                    "on_enter can only be used in state transition stages",
                    (size_t)node->m_sys_addr, node->m_stage.m_stage->name()
                );
            }
            return *this;
        }
        template <typename T>
        SystemInfo& on_exit(T state) {
            if (app->m_runner->stage_state_transition(node->m_stage.m_stage)) {
                node->m_conditions.emplace_back(
                    std::make_unique<Condition<State<T>, NextState<T>>>(
                        [state](State<T> s, NextState<T> ns) {
                            return !ns.is_state(state) && s.is_state(state);
                        }
                    )
                );
            } else {
                spdlog::warn(
                    "adding system {:#018x} to stage {} is not allowed"
                    "on_exit can only be used in state transition stages",
                    (size_t)node->m_sys_addr, node->m_stage.m_stage->name()
                );
            }
            return *this;
        }
        template <typename T>
        SystemInfo& on_change() {
            if (app->m_runner->stage_state_transition(node->m_stage.m_stage)) {
                node->m_conditions.emplace_back(
                    std::make_unique<Condition<State<T>, NextState<T>>>(
                        [](State<T> s, NextState<T> ns) {
                            return !s.is_state(ns);
                        }
                    )
                );
            } else {
                spdlog::warn(
                    "adding system {:#018x} to stage {} is not allowed"
                    "on_change can only be used in state transition stages",
                    (size_t)node->m_sys_addr, node->m_stage.m_stage->name()
                );
            }
            return *this;
        }
        template <typename T>
        SystemInfo& in_state(T state) {
            node->m_conditions.emplace_back(
                std::make_unique<Condition<State<T>>>([state](State<T> s) {
                    return s.is_state(state);
                })
            );
            return *this;
        }
    };

    template <typename StageT, typename... Args>
    SystemInfo add_system(StageT stage, void (*func)(Args...)) {
        SystemNode* ptr = m_runner->add_system(stage, func);
        return {ptr, this};
    }
    template <typename T>
    void add_plugin(T&& plugin) {
        m_plugins.emplace(
            &typeid(T), std::make_shared<T>(std::forward<T>(plugin))
        );
    }
    template <typename T>
    T& get_plugin() {
        return *static_cast<T*>(m_plugins.at(&typeid(T)).get());
    }

    void run() {
        spdlog::info("Building");
        build();
        spdlog::info("Startup");
        m_runner->run_startup();
        end_commands();
        do {
            spdlog::info("State transition");
            m_runner->run_state_transition();
            update_states();
            spdlog::info("Loop");
            m_runner->run_loop();
            tick_events();
            end_commands();
        } while (m_loop_enabled &&
                 !m_check_exit_func->run(
                     m_sub_apps->at(&typeid(MainSubApp)).get(),
                     m_sub_apps->at(&typeid(MainSubApp)).get()
                 ));
        spdlog::info("State transition");
        m_runner->run_state_transition();
        end_commands();
        update_states();
        spdlog::info("Exit");
        m_runner->run_exit();
        end_commands();
    }

    void enable_loop() { m_loop_enabled = true; }
    void disable_loop() { m_loop_enabled = false; }

   protected:
    App()
        : m_sub_apps(std::make_unique<spp::sparse_hash_map<
                         const type_info*,
                         std::unique_ptr<SubApp>>>()),
          m_runner(std::make_unique<Runner>(m_sub_apps.get())) {
        m_check_exit_func = std::make_unique<Condition<EventReader<AppExit>>>(
            [](EventReader<AppExit> reader) {
                for (auto&& evt : reader.read()) {
                    return true;
                }
                return false;
            }
        );
        add_sub_app<MainSubApp>();
    }
    App(const App&)            = delete;
    App& operator=(const App&) = delete;
    App(App&&)                 = default;
    App& operator=(App&&)      = default;

    void build_plugins() {
        for (auto& [ptr, plugin] : m_plugins) {
            plugin->build(*this);
            for (auto& [ptr, subapp] : *m_sub_apps) {
                subapp->m_world.m_resources.emplace(ptr, plugin);
            }
        }
    }
    void build() {
        build_plugins();
        m_runner->build();
        m_runner->bake_all();
    }
    template <typename T>
    void add_sub_app() {
        m_sub_apps->emplace(&typeid(T), std::make_unique<SubApp>());
    }

    void end_commands() {
        for (auto& [ptr, subapp] : *m_sub_apps) {
            subapp->end_commands();
        }
    }

    void tick_events() {
        for (auto& [ptr, subapp] : *m_sub_apps) {
            subapp->tick_events();
        }
    }

    void update_states() {
        for (auto& [ptr, subapp] : *m_sub_apps) {
            subapp->update_states();
        }
    }

    std::unique_ptr<
        spp::sparse_hash_map<const type_info*, std::unique_ptr<SubApp>>>
        m_sub_apps;
    std::unique_ptr<Runner> m_runner;
    spp::sparse_hash_map<const type_info*, std::shared_ptr<Plugin>> m_plugins;
    std::unique_ptr<BasicSystem<bool>> m_check_exit_func;
    bool m_loop_enabled = false;
};
}  // namespace pixel_engine::app