#pragma once

namespace entity {
    class App;

    template <typename Ret>
    class BasicSystem {
       protected:
        App* app;

       public:
        BasicSystem(App* app) : app(app) {}
        virtual Ret run() { return Ret(); };
    };

    template <typename... Args>
    class System : public BasicSystem<void> {
       private:
        void (*func)(Args...);

       public:
        System(App* app, void (*func)(Args...))
            : BasicSystem(app), func(func) {}
        void run() { app->run_system(func); }
    };

    template <typename... Args>
    class Condition : public BasicSystem<bool> {
        friend class App;

       private:
        bool (*func)(Args...);

       public:
        Condition(App* app, bool (*func)(Args...))
            : BasicSystem(app), func(func) {}
        bool run() { return app->run_system_v(func); }
    };
}  // namespace entity