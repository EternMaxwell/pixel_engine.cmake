cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_imgui STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine_imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixel_engine_imgui pixel_engine::app)
target_link_libraries(pixel_engine_imgui pixel_engine::window)
target_link_libraries(pixel_engine_imgui pixel_engine::render::ogl)
target_link_libraries(pixel_engine_imgui pixel_engine::render::vulkan)
target_link_libraries(pixel_engine_imgui imgui)

add_library(pixel_engine::imgui ALIAS pixel_engine_imgui)