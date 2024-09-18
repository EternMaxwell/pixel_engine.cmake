cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_window STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine_window PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixel_engine_window pixel_engine::app)
target_link_libraries(pixel_engine_window glfw)
target_link_libraries(pixel_engine_window OpenGL::GL)
target_link_libraries(pixel_engine_window glad)
target_link_libraries(pixel_engine_window Vulkan::Vulkan)

add_library(pixel_engine::window ALIAS pixel_engine_window)