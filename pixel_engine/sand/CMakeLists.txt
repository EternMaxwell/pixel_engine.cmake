cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_falling_sand STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})
target_include_directories(pixel_engine_falling_sand PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixel_engine_falling_sand pixel_engine::app)
target_link_libraries(pixel_engine_falling_sand glm::glm)

add_library(pixel_engine::world::sand ALIAS pixel_engine_falling_sand)