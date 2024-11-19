#pragma once

#include "imgui/systems.h"

namespace pixel_engine::imgui {
using namespace pixel_engine::prelude;
struct ImGuiPluginVK : Plugin {
    void build(App& app);
};
}  // namespace pixel_engine::imgui