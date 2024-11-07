cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_font STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine_font PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixel_engine_font freetype)
target_link_libraries(pixel_engine_font Vulkan::Vulkan)
target_link_libraries(pixel_engine_font glfw)
target_link_libraries(pixel_engine_font pixel_engine::app)
target_link_libraries(pixel_engine_font pixel_engine::window)
target_link_libraries(pixel_engine_font pixel_engine::render_vk)
target_link_libraries(pixel_engine_font pixel_engine::render::ogl)

add_library(pixel_engine::font ALIAS pixel_engine_font)