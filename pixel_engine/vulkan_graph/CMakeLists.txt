cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_vulkan_graph STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine_vulkan_graph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixel_engine_vulkan_graph EnTT::EnTT)
target_link_libraries(pixel_engine_vulkan_graph BSThreadPool)
target_link_libraries(pixel_engine_vulkan_graph spdlog::spdlog)

add_library(pixel_engine::vulkan_graph ALIAS pixel_engine_vulkan_graph)
