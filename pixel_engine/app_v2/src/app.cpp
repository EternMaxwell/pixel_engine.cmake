#include "pixel_engine/app.h"

using namespace pixel_engine::app;

App* App::SystemInfo::operator->() { return app; }

App App::create() {
    App app;
    app.add_sub_app<RenderSubApp>();
    app.m_runner->assign_startup_stage<MainSubApp, MainSubApp>(
        PreStartup, Startup, PostStartup
    );
    app.m_runner->assign_state_transition_stage<MainSubApp, MainSubApp>(
        Transit
    );
    app.m_runner->assign_loop_stage<MainSubApp, MainSubApp>(
        First, PreUpdate, Update, PostUpdate, Last
    );
    app.m_runner
        ->assign_loop_stage<MainSubApp, MainSubApp>(
            Prepare, PreRender, Render, PostRender
        )
        ->add_prev_stage<MainLoopStage>();
    // this is temporary, currently the render stages are still in main
    // subapp
    app.m_runner->assign_exit_stage<MainSubApp, MainSubApp>(
        PreExit, Exit, PostExit
    );
    return std::move(app);
}
void App::run() {
    m_logger->info("Building App");
    build();
    m_logger->info("Running App");
    m_logger->debug("Startup stage");
    m_runner->run_startup();
    end_commands();
    do {
        m_logger->debug("Transition stage");
        m_runner->run_state_transition();
        update_states();
        m_logger->debug("Loop stage");
        m_runner->run_loop();
        tick_events();
        end_commands();
    } while (m_loop_enabled && !m_check_exit_func->run(
                                   m_sub_apps->at(std::type_index(typeid(MainSubApp))).get(),
                                   m_sub_apps->at(std::type_index(typeid(MainSubApp))).get()
                               ));
    m_logger->debug("Transition stage");
    m_runner->run_state_transition();
    end_commands();
    update_states();
    m_logger->info("Exiting App");
    m_logger->debug("Exit stage");
    m_runner->run_exit();
    end_commands();
    m_logger->info("App terminated");
}
void App::set_log_level(spdlog::level::level_enum level) {
    m_logger->set_level(level);
    m_runner->set_log_level(level);
}
App& App::enable_loop() {
    m_loop_enabled = true;
    return *this;
}
App& App::disable_loop() {
    m_loop_enabled = false;
    return *this;
}
App* App::operator->() { return this; }

App::App()
    : m_sub_apps(
          std::make_unique<
              spp::sparse_hash_map<std::type_index, std::unique_ptr<SubApp>>>()
      ),
      m_runner(std::make_unique<Runner>(m_sub_apps.get())) {
    m_logger = spdlog::default_logger()->clone("app");
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
void App::build_plugins() {
    for (auto& [ptr, plugin] : m_plugins) {
        plugin->build(*this);
        for (auto& [app_ptr, subapp] : *m_sub_apps) {
            subapp->m_world.m_resources.emplace(ptr, plugin);
        }
    }
}
void App::build() {
    build_plugins();
    m_runner->build();
    m_runner->bake_all();
}
void App::end_commands() {
    for (auto& [ptr, subapp] : *m_sub_apps) {
        subapp->end_commands();
    }
}
void App::tick_events() {
    for (auto& [ptr, subapp] : *m_sub_apps) {
        subapp->tick_events();
    }
}
void App::update_states() {
    for (auto& [ptr, subapp] : *m_sub_apps) {
        subapp->update_states();
    }
}