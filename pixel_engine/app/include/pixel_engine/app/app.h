#pragma once

#include <spdlog/spdlog.h>

#include <BS_thread_pool.hpp>
#include <entt/entity/registry.hpp>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "command.h"
#include "event.h"
#include "query.h"
#include "resource.h"
#include "system.h"
#include "tools.h"
#include "world.h"

namespace pixel_engine {
namespace app {

static std::shared_ptr<spdlog::logger> setup_logger =
    spdlog::default_logger()->clone("app-setup");

static std::shared_ptr<spdlog::logger> build_logger =
    spdlog::default_logger()->clone("app-build");

static std::shared_ptr<spdlog::logger> run_logger =
    spdlog::default_logger()->clone("app-run");

enum BasicStage { Start, StateTransition, Loop, Exit };

enum StartStage { PreStartup, Startup, PostStartup };

enum StateTransitionStage { StateTransit };

enum LoopStage { First, PreUpdate, Update, PostUpdate, Last };

enum RenderStage { Prepare, PreRender, Render, PostRender };

enum ExitStage { PreShutdown, Shutdown, PostShutdown };

struct SystemStage {
    const type_info* m_stage_type;
    size_t m_stage_value;
    SystemStage() = default;
    template <typename T>
    SystemStage(T t) {
        m_stage_type = &typeid(T);
        m_stage_value = static_cast<size_t>(t);
    }
};

struct SystemSet {
    const type_info* m_set_type_hash;
    size_t m_set_value;
    template <typename T>
    SystemSet(T t) {
        m_set_type_hash = &typeid(T);
        m_set_value = static_cast<size_t>(t);
    }
};

struct SetMap {
    std::unordered_map<const type_info*, std::vector<size_t>> m_sets;
    template <typename T, typename... Args>
    void configure_sets(T t, Args... args) {
        std::vector<size_t> set = {
            static_cast<size_t>(t), static_cast<size_t>(args)...
        };
        m_sets[&typeid(T)] = set;
    }
};

struct SystemNode {
    SystemStage m_stage;
    std::unordered_set<std::shared_ptr<BasicSystem<bool>>> m_conditions;
    std::vector<SystemSet> m_in_sets;
    std::vector<void*> m_func_addrs;
    std::string m_worker_name = "default";
    std::vector<std::shared_ptr<BasicSystem<void>>> m_system;
    std::unordered_set<void*> m_system_ptrs_before;
    std::unordered_set<void*> m_system_ptrs_after;
    std::unordered_set<std::weak_ptr<SystemNode>> m_systems_before;
    std::unordered_set<std::weak_ptr<SystemNode>> m_systems_after;
    std::unordered_set<std::weak_ptr<SystemNode>> m_temp_before;
    std::unordered_set<std::weak_ptr<SystemNode>> m_temp_after;
    std::optional<double> m_reach_time;

    size_t m_prev_cout;
    size_t m_next_cout;

    SystemNode() = default;
    template <typename... Args>
    SystemNode(void (*func)(Args...)) {
        m_system = {std::make_shared<System<Args...>>(func)};
        m_func_addrs = {func};
    }
    template <typename T, typename... Args>
    SystemNode(T stage, void (*func)(Args...)) : m_stage(stage) {
        m_system = {std::make_shared<System<Args...>>(func)};
        m_func_addrs = {func};
    }
    template <typename... Args>
    SystemNode(std::tuple<Args...> funcs) {
        m_system = std::apply(
            [this](auto&&... args) {
                m_func_addrs = {args...};
                return std::vector<std::shared_ptr<BasicSystem<void>>>{
                    SystemNode::create_system(args)...
                };
            },
            funcs
        );
    }
    template <typename T, typename... Args>
    SystemNode(T stage, std::tuple<Args...> funcs) : m_stage(stage) {
        m_system = std::apply(
            [this](auto&&... args) {
                m_func_addrs = {args...};
                return std::vector<std::shared_ptr<BasicSystem<void>>>{
                    SystemNode::create_system(args)...
                };
            },
            funcs
        );
    }

    template <typename... Args>
    static std::shared_ptr<BasicSystem<void>> create_system(void (*func)(Args...
    )) {
        return std::make_shared<System<Args...>>(func);
    }

    bool contrary_to(SystemNode* other);

    bool condition_pass(SubApp* src, SubApp* dst);

    void run(SubApp* src, SubApp* dst);

    double reach_time();

    void reset_reach_time();

    void clear_temp();
};

template <typename T>
struct MsgQueueBase {
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    MsgQueueBase() = default;
    MsgQueueBase(const MsgQueueBase& other) : m_queue(other.m_queue) {}
    auto& operator=(const MsgQueueBase& other) {
        m_queue = other.m_queue;
        return *this;
    }
    void push(T system) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(system);
        m_cv.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        T system = m_queue.front();
        m_queue.pop();
        return system;
    }
    T front() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.front();
    }
    bool empty() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    void wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_queue.empty(); });
    }
};

struct MsgQueue : MsgQueueBase<std::weak_ptr<SystemNode>> {};

// new implementation should be runners -> stage -> substage,
// where runners are startup, loop and exit.
// stages can be assigned to a runner, and stage is distinguished by
// the enum type. The assignment function should be like:
//     Runners::assign_<runner>_stage(src, dst, stage, substages...);
// where src and dst are SubApp objects, stage is the enum type of the
// stage, and substages are the enum type of the substage.
// which returns a AddStageReturn object, which can be used to set
// the dependencies map of the stages in the runner.

struct SubStageRunner {
   private:
    std::shared_ptr<SubApp> m_src;
    std::shared_ptr<SubApp> m_dst;
    const type_info* m_stage_type;
    size_t m_stage_value;
    std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>*
        m_workers;
    std::unordered_map<const type_info*, std::vector<size_t>>* m_sets;
    std::vector<std::weak_ptr<SystemNode>> m_systems;

    std::shared_ptr<SystemNode> m_head = std::make_shared<SystemNode>();

    MsgQueue m_msg_queue;

   public:
    template <typename StageT>
    SubStageRunner(
        std::shared_ptr<SubApp> src,
        std::shared_ptr<SubApp> dst,
        StageT stage,
        std::unordered_map<const type_info*, std::vector<size_t>>* sets,
        std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>*
            workers
    )
        : m_src(src),
          m_dst(dst),
          m_stage_type(&typeid(StageT)),
          m_stage_value(static_cast<size_t>(stage)),
          m_sets(sets),
          m_workers(workers) {}
    void build(
        const std::unordered_map<void*, std::shared_ptr<SystemNode>>& systems
    );
    void prepare();
    void notify(std::weak_ptr<SystemNode> system);
    void run(std::weak_ptr<SystemNode> system);
    void run();
};

struct StageRunner {
    std::shared_ptr<SubApp> m_src;
    std::shared_ptr<SubApp> m_dst;
    std::vector<SubStageRunner> m_sub_stage_runners;
    const type_info* m_stage_type;
    std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>*
        m_workers;
    std::unordered_map<const type_info*, std::vector<size_t>>* m_sets;
    StageRunner(
        std::shared_ptr<SubApp> src,
        std::shared_ptr<SubApp> dst,
        std::unordered_map<const type_info*, std::vector<size_t>>* sets,
        std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>*
            workers
    );
    template <typename Stage, typename... SubStages>
    void assign_sub_stages(Stage stage, SubStages... substages) {
        auto stages = {stage, substages...};
        for (auto sub_stage : stages) {
            m_sub_stage_runners.emplace_back(
                m_src, m_dst, sub_stage, m_sets, m_workers
            );
        }
        m_stage_type = &typeid(Stage);
    }
    void build(
        const std::unordered_map<void*, std::shared_ptr<SystemNode>>& systems
    );
    void prepare();
    void run();
};

struct StageRunnerInfo {
    std::shared_ptr<StageRunner> m_stage_runner;
    std::unordered_set<std::weak_ptr<StageRunnerInfo>> m_before;
    std::unordered_set<std::weak_ptr<StageRunnerInfo>> m_before_temp;
    std::unordered_set<std::weak_ptr<StageRunnerInfo>> m_after;
    std::unordered_set<std::weak_ptr<StageRunnerInfo>> m_after_temp;
    size_t before_count = 0;
    std::optional<size_t> m_depth;

    double m_run_time_rate = 0.0;

    double m_current_run_time_rate = 0.5;

    StageRunnerInfo(
        std::shared_ptr<SubApp> src,
        std::shared_ptr<SubApp> dst,
        std::unordered_map<const type_info*, std::vector<size_t>>* sets,
        std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>>*
            workers
    )
        : m_stage_runner(std::make_shared<StageRunner>(src, dst, sets, workers)
          ) {}
    StageRunnerInfo() = default;
    auto operator->() -> StageRunner* { return m_stage_runner.get(); }
    void reset() {
        m_before_temp.clear();
        m_after_temp.clear();
        m_depth.reset();
        before_count = 0;
    }
    bool conflict(std::shared_ptr<StageRunnerInfo> other) {
        if (m_stage_runner->m_src == other->m_stage_runner->m_src ||
            m_stage_runner->m_dst == other->m_stage_runner->m_dst ||
            m_stage_runner->m_src == other->m_stage_runner->m_dst ||
            m_stage_runner->m_dst == other->m_stage_runner->m_src) {
            return true;
        }
        return false;
    }
    size_t depth() {
        if (m_depth) {
            return *m_depth;
        }
        size_t max_depth = -1;
        for (auto& before : m_before) {
            size_t depth = before.lock()->depth();
            if (depth > max_depth) {
                max_depth = depth;
            }
        }
        m_depth = max_depth + 1;
        return *m_depth;
    }
};

struct StageRunnerMsgQueue : MsgQueueBase<std::shared_ptr<StageRunnerInfo>> {};

struct Runner {
    std::unordered_map<const type_info*, std::shared_ptr<SubApp>>* m_sub_apps;
    std::unordered_map<std::string, std::shared_ptr<BS::thread_pool>> m_workers;
    std::unordered_map<const type_info*, std::shared_ptr<StageRunnerInfo>>
        m_startup_stages;
    std::unordered_map<const type_info*, std::shared_ptr<StageRunnerInfo>>
        m_transition_stages;
    std::unordered_map<const type_info*, std::shared_ptr<StageRunnerInfo>>
        m_loop_stages;
    std::unordered_map<const type_info*, std::shared_ptr<StageRunnerInfo>>
        m_exit_stages;
    std::vector<std::shared_ptr<StageRunnerInfo>> m_startup_stages_vec;
    std::vector<std::shared_ptr<StageRunnerInfo>> m_transition_stages_vec;
    std::vector<std::shared_ptr<StageRunnerInfo>> m_loop_stages_vec;
    std::vector<std::shared_ptr<StageRunnerInfo>> m_exit_stages_vec;
    std::unordered_map<void*, std::shared_ptr<SystemNode>> m_systems;
    std::unordered_map<const type_info*, std::vector<size_t>> m_sets;
    BS::thread_pool m_runner_pool;
    StageRunnerMsgQueue m_msg_queue;

    double m_run_time_rate = 0.0;

    struct AddStageReturn {
       private:
        Runner* m_runner;
        const type_info* m_stage_type;
        std::unordered_map<const type_info*, std::shared_ptr<StageRunnerInfo>>*
            m_stages;

       public:
        AddStageReturn(
            const type_info* stage_type,
            Runner* runner,
            std::unordered_map<
                const type_info*,
                std::shared_ptr<StageRunnerInfo>>* stages
        )
            : m_stage_type(stage_type), m_stages(stages), m_runner(runner) {}
        template <typename Stage>
        AddStageReturn& before() {
            auto this_stage = m_stages->find(m_stage_type);
            if (this_stage != m_stages->end()) {
                auto stage = m_stages->find(&typeid(Stage));
                if (stage != m_stages->end()) {
                    this_stage->second->m_after.insert(stage->second);
                    stage->second->m_before.insert(this_stage->second);
                } else {
                    setup_logger->warn(
                        "Trying to add stage {} before stage {}, which does "
                        "not exist.",
                        typeid(Stage).name(), m_stage_type->name()
                    );
                }
            }
            return *this;
        }
        template <typename Stage>
        AddStageReturn& after() {
            auto this_stage = m_stages->find(m_stage_type);
            if (this_stage != m_stages->end()) {
                auto stage = m_stages->find(&typeid(Stage));
                if (stage != m_stages->end()) {
                    this_stage->second->m_before.insert(stage->second);
                    stage->second->m_after.insert(this_stage->second);
                } else {
                    setup_logger->warn(
                        "Trying to add stage {} after stage {}, which does not "
                        "exist.",
                        typeid(Stage).name(), m_stage_type->name()
                    );
                }
            }
            return *this;
        }
    };
    template <typename Stage, typename... SubStages>
    AddStageReturn assign_startup_stage(
        std::shared_ptr<SubApp> src,
        std::shared_ptr<SubApp> dst,
        Stage stage,
        SubStages... substages
    ) {
        auto stage_runner =
            std::make_shared<StageRunnerInfo>(src, dst, &m_sets, &m_workers);
        stage_runner->m_stage_runner->assign_sub_stages(stage, substages...);
        m_startup_stages.insert({&typeid(Stage), stage_runner});
        m_startup_stages_vec.push_back(stage_runner);
        return AddStageReturn(&typeid(Stage), this, &m_startup_stages);
    }
    template <
        typename SrcT,
        typename DstT,
        typename Stage,
        typename... SubStages>
    AddStageReturn assign_startup_stage(Stage stage, SubStages... substages) {
        auto src = m_sub_apps->find(&typeid(SrcT));
        auto dst = m_sub_apps->find(&typeid(DstT));
        if (src == m_sub_apps->end() || dst == m_sub_apps->end()) {
            setup_logger->warn(
                "Trying to assign startup stage from {} to {}, which does not "
                "exist.",
                typeid(SrcT).name(), typeid(DstT).name()
            );
            return AddStageReturn(&typeid(Stage), this, &m_startup_stages);
        }
        return assign_startup_stage(
            src->second, dst->second, stage, substages...
        );
    }
    template <typename Stage, typename... SubStages>
    AddStageReturn assign_transition_stage(
        std::shared_ptr<SubApp> src,
        std::shared_ptr<SubApp> dst,
        Stage stage,
        SubStages... substages
    ) {
        auto stage_runner =
            std::make_shared<StageRunnerInfo>(src, dst, &m_sets, &m_workers);
        stage_runner->m_stage_runner->assign_sub_stages(stage, substages...);
        m_transition_stages.insert({&typeid(Stage), stage_runner});
        m_transition_stages_vec.push_back(stage_runner);
        return AddStageReturn(&typeid(Stage), this, &m_transition_stages);
    }
    template <
        typename SrcT,
        typename DstT,
        typename Stage,
        typename... SubStages>
    AddStageReturn assign_transition_stage(
        Stage stage, SubStages... substages
    ) {
        auto src = m_sub_apps->find(&typeid(SrcT));
        auto dst = m_sub_apps->find(&typeid(DstT));
        if (src == m_sub_apps->end() || dst == m_sub_apps->end()) {
            setup_logger->warn(
                "Trying to assign transition stage from {} to {}, which does "
                "not "
                "exist.",
                typeid(SrcT).name(), typeid(DstT).name()
            );
            return AddStageReturn(&typeid(Stage), this, &m_transition_stages);
        }
        return assign_transition_stage(
            src->second, dst->second, stage, substages...
        );
    }
    template <typename Stage, typename... SubStages>
    AddStageReturn assign_loop_stage(
        std::shared_ptr<SubApp> src,
        std::shared_ptr<SubApp> dst,
        Stage stage,
        SubStages... substages
    ) {
        auto stage_runner =
            std::make_shared<StageRunnerInfo>(src, dst, &m_sets, &m_workers);
        stage_runner->m_stage_runner->assign_sub_stages(stage, substages...);
        m_loop_stages.insert({&typeid(Stage), stage_runner});
        m_loop_stages_vec.push_back(stage_runner);
        return AddStageReturn(&typeid(Stage), this, &m_loop_stages);
    }
    template <
        typename SrcT,
        typename DstT,
        typename Stage,
        typename... SubStages>
    AddStageReturn assign_loop_stage(Stage stage, SubStages... substages) {
        auto src = m_sub_apps->find(&typeid(SrcT));
        auto dst = m_sub_apps->find(&typeid(DstT));
        if (src == m_sub_apps->end() || dst == m_sub_apps->end()) {
            setup_logger->warn(
                "Trying to assign loop stage from {} to {}, which does not "
                "exist.",
                typeid(SrcT).name(), typeid(DstT).name()
            );
            return AddStageReturn(&typeid(Stage), this, &m_loop_stages);
        }
        return assign_loop_stage(src->second, dst->second, stage, substages...);
    }
    template <typename Stage, typename... SubStages>
    AddStageReturn assign_exit_stage(
        std::shared_ptr<SubApp> src,
        std::shared_ptr<SubApp> dst,
        Stage stage,
        SubStages... substages
    ) {
        auto stage_runner =
            std::make_shared<StageRunnerInfo>(src, dst, &m_sets, &m_workers);
        stage_runner->m_stage_runner->assign_sub_stages(stage, substages...);
        m_exit_stages.insert({&typeid(Stage), stage_runner});
        m_exit_stages_vec.push_back(stage_runner);
        return AddStageReturn(&typeid(Stage), this, &m_exit_stages);
    }
    template <
        typename SrcT,
        typename DstT,
        typename Stage,
        typename... SubStages>
    AddStageReturn assign_exit_stage(Stage stage, SubStages... substages) {
        auto src = m_sub_apps->find(&typeid(SrcT));
        auto dst = m_sub_apps->find(&typeid(DstT));
        if (src == m_sub_apps->end() || dst == m_sub_apps->end()) {
            setup_logger->warn(
                "Trying to assign exit stage from {} to {}, which does not "
                "exist.",
                typeid(SrcT).name(), typeid(DstT).name()
            );
            return AddStageReturn(&typeid(Stage), this, &m_exit_stages);
        }
        return assign_exit_stage(src->second, dst->second, stage, substages...);
    }
    template <typename T>
    bool stage_is_startup() {
        return m_startup_stages.find(&typeid(T)) != m_startup_stages.end();
    }
    bool stage_is_startup(const type_info* type);
    template <typename T>
    bool stage_is_transition() {
        return m_transition_stages.find(&typeid(T)) !=
               m_transition_stages.end();
    }
    bool stage_is_transition(const type_info* type);
    template <typename T>
    bool stage_is_loop() {
        return m_loop_stages.find(&typeid(T)) != m_loop_stages.end();
    }
    bool stage_is_loop(const type_info* type);
    template <typename T>
    bool stage_is_exit() {
        return m_exit_stages.find(&typeid(T)) != m_exit_stages.end();
    }
    bool stage_is_exit(const type_info* type);
    template <typename T, typename... Sets>
    void configure_sets(T set, Sets... sets) {
        m_sets[&typeid(T)] = {
            static_cast<size_t>(set), static_cast<size_t>(sets)...
        };
    }
    Runner(
        std::unordered_map<const type_info*, std::shared_ptr<SubApp>>* sub_apps,
        std::vector<std::pair<std::string, size_t>> workers
    );
    void add_worker(std::string name, int num_threads);
    std::shared_ptr<StageRunnerInfo> prepare(
        std::vector<std::shared_ptr<StageRunnerInfo>>& stages
    );
    void run(std::shared_ptr<StageRunnerInfo> stage);
    void run(
        std::vector<std::shared_ptr<StageRunnerInfo>>& stages,
        std::shared_ptr<StageRunnerInfo> head
    );
    void run_startup();
    void run_transition();
    void run_loop();
    void run_exit();
    void build();
    void end_commands();
    void removing_system(void* func);
    void set_run_time_rate(double rate);

    template <typename... Args>
    SystemNode* add_system(void (*func)(Args...)) {
        if (m_systems.find(func) != m_systems.end()) {
            setup_logger->warn(
                "Trying to add system {:#016x} : {}, which already exists.",
                (size_t)func, typeid(func).name()
            );
            return nullptr;
        }
        std::shared_ptr<SystemNode> node = std::make_shared<SystemNode>(func);
        m_systems.insert({(void*)func, node});
        return node.get();
    }
    template <typename T, typename... Args>
    SystemNode* add_system(T stage, void (*func)(Args...)) {
        if (m_systems.find(func) != m_systems.end()) {
            setup_logger->warn(
                "Trying to add system {:#016x} : {}, which already exists.",
                (size_t)func, typeid(func).name()
            );
            return nullptr;
        }
        std::shared_ptr<SystemNode> node =
            std::make_shared<SystemNode>(stage, func);
        m_systems.insert({(void*)func, node});
        return node.get();
    }

    template <typename... Args>
    SystemNode* add_system(std::tuple<Args...> funcs) {
        static_assert(
            sizeof...(Args) > 0,
            "System tuple must have at least 1 "
            "system."
        );
        std::shared_ptr<SystemNode> node = std::make_shared<SystemNode>(funcs);
        bool exists = false;
        std::string warn;
        for (auto func : node->m_func_addrs) {
            if (m_systems.find(func) != m_systems.end()) {
                warn += fmt::format(
                    "\tSystem {:#016x} : {}, ", (size_t)func,
                    typeid(func).name()
                );
                exists = true;
            }
        }
        if (exists) {
            setup_logger->warn(
                "Trying to add system tuple with systems already existing:\n{}",
                warn
            );
            return nullptr;
        }
        for (auto func : node->m_func_addrs) {
            m_systems.insert({func, node});
        }
        return node.get();
    }
    template <typename T, typename... Args>
    SystemNode* add_system(T stage, std::tuple<Args...> funcs) {
        static_assert(
            sizeof...(Args) > 0,
            "System tuple must have at least 1 "
            "system."
        );
        std::shared_ptr<SystemNode> node =
            std::make_shared<SystemNode>(stage, funcs);
        bool exists = false;
        std::string warn;
        for (auto func : node->m_func_addrs) {
            if (m_systems.find(func) != m_systems.end()) {
                warn += fmt::format(
                    "\tSystem {:#016x} : {}, ", (size_t)func,
                    typeid(func).name()
                );
                exists = true;
            }
        }
        if (exists) {
            setup_logger->warn(
                "Trying to add system tuple with systems already existing:\n{}",
                warn
            );
            return nullptr;
        }
        for (auto func : node->m_func_addrs) {
            m_systems.insert({func, node});
        }
        return node.get();
    }
};

struct AppExit {};

bool check_exit(EventReader<AppExit> reader);

struct App;

struct Plugin {
    virtual void build(App& app) = 0;
};

struct App {
    auto operator=(const App&) -> App& = delete;
    auto operator=(App&&) -> App& = delete;
    auto operator=(const App&&) -> App& = delete;
    App(const App&) = delete;
    App(App&&) = delete;
    App(const App&&) = delete;
    enum Loggers { Setup, Build, Run };
    struct PluginCache {
       private:
        const type_info* m_plugin_type;
        std::unordered_set<void*> m_system_ptrs;

       public:
        PluginCache() = default;
        template <typename T>
        PluginCache(T&& t) {
            m_plugin_type = &typeid(T);
        }
        void add_system(void* system_ptr);
    };

    struct AddSystemReturn {
       private:
        SystemNode* m_system;
        App* m_app;

       public:
        AddSystemReturn(App* app, SystemNode* sys);
        App* operator->();
        template <typename... Sets>
        AddSystemReturn& in_set(Sets... sets) {
            if (m_system) {
                std::array<SystemSet, sizeof...(Sets)> sets_container = {
                    SystemSet(sets)...
                };
                for (auto& sys_set : sets_container) {
                    m_system->m_in_sets.push_back(sys_set);
                }
            }
            return *this;
        }
        template <typename... Addrs>
        AddSystemReturn& before(Addrs... addrs) {
            if (m_system) {
                std::vector<void*> addrs_v = {((void*)addrs)...};
                for (auto ptr : addrs_v) {
                    m_system->m_system_ptrs_after.insert(ptr);
                }
            }
            return *this;
        }
        template <typename... Addrs>
        AddSystemReturn& after(Addrs... addrs) {
            if (m_system) {
                std::vector<void*> addrs_v = {((void*)addrs)...};
                for (auto ptr : addrs_v) {
                    m_system->m_system_ptrs_before.insert(ptr);
                }
            }
            return *this;
        }
        template <typename T>
        AddSystemReturn& in_stage(T stage) {
            if (m_system) {
                m_system->m_stage = SystemStage(stage);
            }
            return *this;
        }
        AddSystemReturn& use_worker(std::string name);
        AddSystemReturn& run_if(std::shared_ptr<BasicSystem<bool>> cond);
        template <typename T>
        AddSystemReturn& on_enter(T state) {
            if (m_system) {
                if (!m_app->m_runner.stage_is_transition(
                        m_system->m_stage.m_stage_type
                    )) {
                    setup_logger->warn(
                        "Trying to add on_enter condition to a system in a "
                        "non-transition stage."
                    );
                    return *this;
                }
                m_system->m_conditions.insert(
                    std::make_shared<
                        Condition<Resource<State<T>>, Resource<NextState<T>>>>(
                        [&](Resource<State<T>> cur,
                            Resource<NextState<T>> next) {
                            return cur->is_just_created() ||
                                   (!cur->is_state(state) &&
                                    next->is_state(state));
                        }
                    )
                );
            }
            return *this;
        }
        template <typename T>
        AddSystemReturn& on_exit(T state) {
            if (m_system) {
                if (!m_app->m_runner.stage_is_transition(
                        m_system->m_stage.m_stage_type
                    )) {
                    setup_logger->warn(
                        "Trying to add on_exit condition to a system in a "
                        "non-transition stage."
                    );
                    return *this;
                }
                m_system->m_conditions.insert(
                    std::make_shared<
                        Condition<Resource<State<T>>, Resource<NextState<T>>>>(
                        [&](Resource<State<T>> cur,
                            Resource<NextState<T>> next) {
                            return cur->is_state(state) &&
                                   !next->is_state(state);
                        }
                    )
                );
            }
            return *this;
        }
        template <typename T>
        AddSystemReturn& on_change() {
            if (m_system) {
                if (!m_app->m_runner.stage_is_transition(
                        m_system->m_stage.m_stage_type
                    )) {
                    setup_logger->warn(
                        "Trying to add on_change condition to a system in a "
                        "non-transition stage."
                    );
                    return *this;
                }
                m_system->m_conditions.insert(
                    std::make_shared<
                        Condition<Resource<State<T>>, Resource<NextState<T>>>>(
                        [&](Resource<State<T>> cur, Resource<NextState<T>> next
                        ) { return !cur->is_state(*next); }
                    )
                );
            }
            return *this;
        }
    };

   private:
    std::unordered_map<const type_info*, std::shared_ptr<SubApp>> m_sub_apps;
    const type_info* m_main_sub_app = nullptr;
    Runner m_runner = Runner(&m_sub_apps, {{"default", 8}, {"single", 1}});
    std::vector<std::shared_ptr<BasicSystem<void>>> m_state_update;
    std::unordered_map<const type_info*, PluginCache> m_plugin_caches;
    std::unordered_map<const type_info*, std::shared_ptr<Plugin>> m_plugins;
    bool m_loop_enabled = false;
    const type_info* m_building_plugin_type = nullptr;

    void update_states();

    struct MainSubApp {};
    struct RenderSubApp {};

    template <typename T>
    void add_sub_app() {
        if (m_sub_apps.find(&typeid(T)) != m_sub_apps.end()) {
            setup_logger->warn(
                "Trying to add sub app {}, which already exists.",
                typeid(T).name()
            );
            return;
        }
        m_sub_apps.insert({&typeid(T), std::make_shared<SubApp>()});
    }

   public:
    App();

    App* operator->();

    App& log_level(Loggers logger, spdlog::level::level_enum level);

    App& log_level(spdlog::level::level_enum level);

    App& enable_loop();

    App& set_run_time_rate(double rate);

    template <typename T, typename... Args>
    App& configure_sets(T set, Args... sets) {
        m_runner.configure_sets(set, sets...);
        return *this;
    }
    template <typename... Args>
    AddSystemReturn add_system(void (*func)(Args...)) {
        auto ptr = m_runner.add_system(func);
        if (m_building_plugin_type && ptr) {
            m_plugin_caches[m_building_plugin_type].add_system((void*)func);
        }
        return AddSystemReturn(this, ptr);
    }
    template <typename T, typename... Args>
    AddSystemReturn add_system(T state, void (*func)(Args...)) {
        auto ptr = m_runner.add_system(state, func);
        if (m_building_plugin_type && ptr) {
            m_plugin_caches[m_building_plugin_type].add_system((void*)func);
        }
        return AddSystemReturn(this, ptr);
    }
    template <typename... Args>
    AddSystemReturn add_system(std::tuple<Args...> funcs) {
        auto ptr = m_runner.add_system(funcs);
        if (m_building_plugin_type && ptr) {
            m_plugin_caches[m_building_plugin_type].add_system(
                (void*)std::get<0>(funcs)
            );
        }
        return AddSystemReturn(this, ptr);
    }
    template <typename T, typename... Args>
    AddSystemReturn add_system(T state, std::tuple<Args...> funcs) {
        auto ptr = m_runner.add_system(state, funcs);
        if (m_building_plugin_type && ptr) {
            m_plugin_caches[m_building_plugin_type].add_system(
                (void*)std::get<0>(funcs)
            );
        }
        return AddSystemReturn(this, ptr);
    }

    template <typename T>
    App& add_plugin(T&& plugin) {
        if (m_plugin_caches.find(&typeid(T)) != m_plugin_caches.end()) {
            setup_logger->warn(
                "Trying to add plugin {}, which already exists.",
                typeid(T).name()
            );
            return *this;
        }
        m_building_plugin_type = &typeid(T);
        PluginCache cache(plugin);
        m_plugin_caches.insert({m_building_plugin_type, cache});
        m_plugins.insert({m_building_plugin_type, std::make_shared<T>(plugin)});
        m_building_plugin_type = nullptr;
        return *this;
    }

    void build_plugins();

    template <typename T>
    App& remove_plugin() {
        if (m_plugin_caches.find(&typeid(T)) != m_plugin_caches.end()) {
            for (auto system_ptr : m_plugin_caches[&typeid(T)].m_system_ptrs) {
                if (m_runner.stage_is_loop(system_ptr->m_stage.m_stage_type) ||
                    m_runner.stage_is_transition(
                        system_ptr->m_stage.m_stage_type
                    ))
                    m_runner.removing_system(system_ptr);
            }
            m_plugin_caches.erase(&typeid(T));
        } else {
            setup_logger->warn(
                "Trying to remove plugin {}, which does not exist.",
                typeid(T).name()
            );
        }
        return *this;
    }

    void run();

    template <typename T>
    App& insert_state(T state) {
        auto& sub_app = m_sub_apps[m_main_sub_app];
        sub_app->insert_state(state);
        return *this;
    }

    template <typename T>
    App& init_state() {
        auto& sub_app = m_sub_apps[m_main_sub_app];
        sub_app->init_state<T>();
        return *this;
    }

    template <
        typename T,
        typename U = std::enable_if_t<std::is_base_of<Plugin, T>::value>>
    auto get_plugin() {
        return std::dynamic_pointer_cast<T>(m_plugins[&typeid(T)]);
    }
};
}  // namespace app
}  // namespace pixel_engine