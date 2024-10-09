#pragma once

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace pixel_engine {
namespace fg {
struct render_pass_base;
struct resource_base;
struct framegraph;

template <typename context_t, typename desc_t, typename actual_t>
std::unique_ptr<actual_t> create_actual(context_t* ctx, desc_t& desc) {
    static_assert(false, "create_actual not implemented for this type");
}

template <typename context_t, typename actual_t>
void destroy_actual(context_t* ctx, std::unique_ptr<actual_t>& actual) {
    static_assert(false, "destroy_actual not implemented for this type");
}

template <typename context_t, typename actual_t, typename usage_t>
void add_barrier(
    context_t* ctx, std::unique_ptr<actual_t>& actual, usage_t& usage
) {
    static_assert(false, "add_barrier not implemented for this type");
}

template <typename context_t, typename actual_t, typename usage_t>
void read_barrier(
    context_t* ctx, std::unique_ptr<actual_t>& actual, usage_t& usage
) {
    add_barrier(ctx, actual, usage);
}

template <typename context_t, typename actual_t, typename usage_t>
void write_barrier(
    context_t* ctx, std::unique_ptr<actual_t>& actual, usage_t& usage
) {
    add_barrier(ctx, actual, usage);
}

template <typename context_t, typename... Args>
std::unique_ptr<context_t> create_context(Args... args) {
    static_assert(false, "create_context not implemented for this type");
}

template <typename context_t>
void destroy_context(std::unique_ptr<context_t>& context) {
    static_assert(false, "destroy_context not implemented for this type");
}

struct resource_base {
   public:
    void read_in(const render_pass_base& pass) { read_passes.insert(&pass); }
    void write_in(const render_pass_base& pass) { write_passes.insert(&pass); }

    auto& get_id() { return id; }
    auto& get_ref_count() { return ref_count; }
    auto& get_name() { return name; }
    void set_name(const std::string& name) { this->name = name; }

    resource_base(const resource_base&) = delete;
    resource_base(resource_base&&) = default;
    resource_base& operator=(const resource_base&) = delete;
    resource_base& operator=(resource_base&&) = default;

   protected:
    friend struct framegraph;

    void realize(framegraph&) {}
    void unrealize(framegraph&) {}
    void read_barrier(framegraph&) {}
    void write_barrier(framegraph&) {}

    size_t id;
    size_t ref_count;
    std::string name;
    std::unordered_set<const render_pass_base*> read_passes;
    std::unordered_set<const render_pass_base*> write_passes;
};
template <
    typename context_t,
    typename desc_t,
    typename actual_t,
    typename usage_t>
struct resource;
struct render_pass_base {
   public:
    void read_resource(resource_base& resource) {
        read_resources.insert(&resource);
        resource.read_in(*this);
    }
    void write_resource(resource_base& resource) {
        write_resources.insert(&resource);
        resource.write_in(*this);
    }

   protected:
    size_t id;
    size_t ref_count;
    std::string name;
    std::unordered_set<const resource_base*> read_resources;
    std::unordered_set<const resource_base*> write_resources;
};
struct framegraph {
   public:
    template <typename context_t, typename... Args>
    void set_context(Args... args) {
        context = create_context<context_t>(args...);
    }
    template <typename context_t>
    auto get_context() {
        return reinterpret_cast<context_t*>(context.get());
    }
    template <typename context_t>
    void destroy_context() {
        destroy_context<context_t>(context);
    }

    template <
        typename context_t,
        typename desc_t,
        typename actual_t,
        typename usage_t>
    resource<context_t, desc_t, actual_t, usage_t>* create_resource(
        const std::string& name, desc_t desc, usage_t usage
    ) {
        size_t id;
        if (free_resources.empty()) {
            id = resources.size();
            resources.emplace_back(
                std::make_unique<
                    resource<context_t, desc_t, actual_t, usage_t>>(
                    name, desc, usage
                )
            );
        } else {
            id = free_resources.top();
            free_resources.pop();
            resources[id] = std::make_unique<
                resource<context_t, desc_t, actual_t, usage_t>>(
                name, desc, usage
            );
        }
        resources[id]->id = id;
        resources[id]->set_name(name);
        resource_map[name] = id;
        return dynamic_cast<resource<context_t, desc_t, actual_t, usage_t>*>(
            resources[id].get()
        );
    }

    resource_base* get_resource(const std::string& name) {
        return resources[resource_map[name]].get();
    }

    void remove_resource(const std::string& name) {
        auto id = resource_map[name];
        resources[id]->unrealize(*this);
        free_resources.push(id);
        resource_map.erase(name);
    }

    void reset_resource(const std::string& name) {
        auto id = resource_map[name];
        resources[id]->unrealize(*this);
    }

   private:
    friend struct resource_base;
    friend struct render_pass_base;

    std::vector<std::unique_ptr<resource_base>> resources;
    std::stack<size_t> free_resources;
    std::unordered_map<std::string, size_t> resource_map;
    std::unique_ptr<void> context;
};
template <
    typename context_t,
    typename desc_t,
    typename actual_t,
    typename usage_t>
struct resource : public resource_base {
   public:
    auto& get_desc() { return desc; }
    auto* get_actual() { return actual.get(); }

    resource(const std::string& name, desc_t desc, usage_t usage)
        : desc(desc), usage(usage) {
        this->name = name;
    }

    resource(const resource&) = delete;
    resource(resource&&) = default;
    resource& operator=(const resource&) = delete;
    resource& operator=(resource&&) = default;

   protected:
    void realize(framegraph& fg) override {
        if (!actual) {
            auto ctx = fg.get_context<context_t>();
            actual = create_actual(ctx, desc);
        }
    }
    void unrealize(framegraph& fg) override {
        if (actual) {
            destroy_actual(fg.get_context<context_t>(), actual);
            actual.reset();
        }
    }
    void read_barrier(framegraph& fg) override {
        if (actual) {
            read_barrier(fg.get_context<context_t>(), actual, usage);
        }
    }
    void write_barrier(framegraph& fg) override {
        if (actual) {
            write_barrier(fg.get_context<context_t>(), actual, usage);
        }
    }

    desc_t desc;
    usage_t usage;
    std::unique_ptr<actual_t> actual;
};
}  // namespace fg
}  // namespace pixel_engine