#ifndef PIXEL_ENGINE_CORE_H_
#define PIXEL_ENGINE_CORE_H_

namespace pixel_engine {
    class Application {
       public:
        Application() {}

        virtual void init() = 0;
        virtual void input() = 0;
        virtual void update() = 0;
        virtual void render() = 0;
        virtual bool running() = 0;
        void loop() {
            while (running()) {
                input();
                update();
                render();
            }
        }
        virtual void dispose() = 0;

        void run() {
            init();
            loop();
            dispose();
        }

        ~Application() {}
    };
}  // namespace pixel_engine

#endif  // PIXEL_ENGINE_CORE_H_