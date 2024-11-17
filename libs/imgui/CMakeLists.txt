cmake_minimum_required(VERSION 3.21)

project(imgui LANGUAGES CXX)

set(IMGUI_SOURCES
    "imgui.cpp"
    "imgui_demo.cpp"
    "imgui_draw.cpp"
    "imgui_widgets.cpp"
    "imgui_tables.cpp"
    "backends/imgui_impl_glfw.cpp"
    "backends/imgui_impl_opengl3.cpp"
    "backends/imgui_impl_vulkan.cpp"
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(imgui PRIVATE glfw)
target_link_libraries(imgui PRIVATE OpenGL::GL)
target_link_libraries(imgui PRIVATE Vulkan::Vulkan)