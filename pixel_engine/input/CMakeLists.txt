cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_input STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine_input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixel_engine_input pixel_engine::app)
target_link_libraries(pixel_engine_input glfw)
target_link_libraries(pixel_engine_input OpenGL::GL)
target_link_libraries(pixel_engine_input glad)
target_link_libraries(pixel_engine_input Vulkan::Vulkan)
target_link_libraries(pixel_engine_input pixel_engine::window)

add_library(pixel_engine::input ALIAS pixel_engine_input)