cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_pixel_render STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine_pixel_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixel_engine_pixel_render freetype)
target_link_libraries(pixel_engine_pixel_render Vulkan::Vulkan)
target_link_libraries(pixel_engine_pixel_render glfw)
target_link_libraries(pixel_engine_pixel_render pixel_engine::app)
target_link_libraries(pixel_engine_pixel_render pixel_engine::window)
target_link_libraries(pixel_engine_pixel_render pixel_engine::render::vulkan)
target_link_libraries(pixel_engine_pixel_render pixel_engine::render::font)

add_library(pixel_engine::render::pixel ALIAS pixel_engine_pixel_render)