#include "pixel_engine/imgui.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine;

EPIX_API ImGuiContext* imgui::ImGuiContext::current_context() {
    return context;
}