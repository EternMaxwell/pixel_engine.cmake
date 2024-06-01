#pragma once

#include "pixel_engine/entity/entity.h"

namespace pixel_engine {
    namespace entity {
        class Scheduler {
           public:
            virtual bool should_run() = 0;
        };

        class Startup : public Scheduler {
           private:
            bool m_value;

           public:
            Startup() : m_value(true), Scheduler() {}
            bool should_run() override {
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
            bool should_run() override { return true; }
        };
    }  // namespace entity
}  // namespace pixel_engine