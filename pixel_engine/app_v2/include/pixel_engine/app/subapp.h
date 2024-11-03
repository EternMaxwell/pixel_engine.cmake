#pragma once

#include <entt/entity/registry.hpp>

#include "command.h"
#include "event.h"
#include "query.h"
#include "resource.h"
#include "world.h"


namespace pixel_engine::app {
struct SubApp {
    template <typename T>
    struct value_type {};

    template <typename ResT>
    struct value_type<Res<ResT>> {
        Res<ResT> operator()(SubApp& app) {
            return Res<ResT>(app.m_world.m_resources[&typeid(ResT)].get());
        }
        Res<ResT> operator()(SubApp& src, SubApp& dst) {
            return Res<ResT>(dst.m_world.m_resources[&typeid(ResT)].get());
        }
    };

    template <typename ResT>
    struct value_type<ResMut<ResT>> {
        ResMut<ResT> operator()(SubApp& app) {
            return ResMut<ResT>(app.m_world.m_resources[&typeid(ResT)].get());
        }
        ResMut<ResT> operator()(SubApp& src, SubApp& dst) {
            return ResMut<ResT>(dst.m_world.m_resources[&typeid(ResT)].get());
        }
    };

    template <>
    struct value_type<Command> {
        Command operator()(SubApp& app) {
            app.m_command_cache.emplace_back(app.m_world);
            return app.m_command_cache.back();
        }
        Command operator()(SubApp& src, SubApp& dst) {
            dst.m_command_cache.emplace_back(dst.m_world);
            return dst.m_command_cache.back();
        }
    };

    template <typename T>
    struct value_type<EventReader<T>> {
        EventReader<T> operator()(SubApp& app) {
            return EventReader<T>(app.m_world.m_event_queues[&typeid(T)].get());
        }
        EventReader<T> operator()(SubApp& src, SubApp& dst) {
            return EventReader<T>(src.m_world.m_event_queues[&typeid(T)].get());
        }
    };

    template <typename T>
    struct value_type<EventWriter<T>> {
        EventWriter<T> operator()(SubApp& app) {
            return EventWriter<T>(app.m_world.m_event_queues[&typeid(T)].get());
        }
        EventWriter<T> operator()(SubApp& src, SubApp& dst) {
            return EventWriter<T>(dst.m_world.m_event_queues[&typeid(T)].get());
        }
    };

    template <typename... Gs, typename... Ws, typename... WOs>
    struct value_type<Query<Get<Gs...>, With<Ws...>, Without<WOs...>>> {
        Query<Get<Gs...>, With<Ws...>, Without<WOs...>> operator()(SubApp& app
        ) {
            return Query<Get<Gs...>, With<Ws...>, Without<WOs...>>(
                app.m_world.m_registry
            );
        }
        Query<Get<Gs...>, With<Ws...>, Without<WOs...>> operator()(
            SubApp& src, SubApp& dst) {
            return Query<Get<Gs...>, With<Ws...>, Without<WOs...>>(
                dst.m_world.m_registry
            );
        }
    };

    template <typename... Gs, typename... Ws, typename... WOs>
    struct value_type<Extract<Get<Gs...>, With<Ws...>, Without<WOs...>>> {
        Extract<Get<Gs...>, With<Ws...>, Without<WOs...>> operator()(SubApp& app
        ) {
            return Extract<Get<Gs...>, With<Ws...>, Without<WOs...>>(
                app.m_world.m_registry
            );
        }
        Extract<Get<Gs...>, With<Ws...>, Without<WOs...>> operator()(
            SubApp& src, SubApp& dst) {
            return Extract<Get<Gs...>, With<Ws...>, Without<WOs...>>(
                src.m_world.m_registry
            );
        }
    };

   public:
    void tick_events() {
        for (auto& [type, queue] : m_world.m_event_queues) {
            queue->tick();
        }
    }
    void end_commands() {
        for (auto& command : m_command_cache) {
            command.end();
        }
        m_command_cache.clear();
    }

   private:
    World m_world;
    std::vector<Command> m_command_cache;
};
}  // namespace pixel_engine::app