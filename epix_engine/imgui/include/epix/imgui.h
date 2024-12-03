#pragma once

#include "imgui/systems.h"

namespace epix::imgui {
using namespace epix::prelude;
struct ImGuiPluginVK : Plugin {
    EPIX_API void build(App& app);
};
}  // namespace epix::imgui