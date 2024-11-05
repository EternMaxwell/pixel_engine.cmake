cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_sprite STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine_sprite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(pixel_engine_sprite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/stb)

target_link_libraries(pixel_engine_sprite freetype)
target_link_libraries(pixel_engine_sprite Vulkan::Vulkan)
target_link_libraries(pixel_engine_sprite glfw)
target_link_libraries(pixel_engine_sprite pixel_engine::app)
target_link_libraries(pixel_engine_sprite pixel_engine::window)
target_link_libraries(pixel_engine_sprite pixel_engine::render_vk)
target_link_libraries(pixel_engine_sprite pixel_engine::font)

add_library(pixel_engine::sprite ALIAS pixel_engine_sprite)