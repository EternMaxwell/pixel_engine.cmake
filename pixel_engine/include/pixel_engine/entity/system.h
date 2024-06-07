#pragma once

namespace pixel_engine {
    namespace entity {
        class App;

        template <typename Ret>
        class BasicSystem {
           protected:
            App* app;

           public:
            BasicSystem(App* app) : app(app) {}
            virtual Ret run() = 0;
        };

        template <typename... Args>
        class System : public BasicSystem<void> {
           private:
            std::function<void(Args...)> func;

           public:
            System(App* app, void (*func)(Args...)) : BasicSystem(app), func(func) {}
            System(App* app, std::function<void(Args...)> func) : BasicSystem(app), func(func) {}
            void run() { app->run_system(func); }
        };

        template <typename... Args>
        class Condition : public BasicSystem<bool> {
           private:
            std::function<bool(Args...)> func;

           public:
            Condition(App* app, bool (*func)(Args...)) : BasicSystem(app), func(func) {}
            Condition(App* app, std::function<bool(Args...)> func) : BasicSystem(app), func(func) {}
            bool run() { return app->run_system_v(func); }
        };
    }  // namespace entity
}  // namespace pixel_engine