cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_physics2d STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})
target_include_directories(pixel_engine_physics2d PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixel_engine_physics2d PUBLIC pixel_engine::app)
target_link_libraries(pixel_engine_physics2d PUBLIC box2d)
target_link_libraries(pixel_engine_physics2d PUBLIC glm::glm)
target_link_libraries(pixel_engine_physics2d PUBLIC earcut.hpp)

add_library(pixel_engine::physics::2d ALIAS pixel_engine_physics2d)