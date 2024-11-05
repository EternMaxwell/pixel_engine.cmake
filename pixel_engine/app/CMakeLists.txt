cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_app_v1 STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine_app_v1 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixel_engine_app_v1 EnTT::EnTT)
target_link_libraries(pixel_engine_app_v1 BSThreadPool)
target_link_libraries(pixel_engine_app_v1 spdlog::spdlog)

add_subdirectory(examples)

add_library(pixel_engine::app1 ALIAS pixel_engine_app_v1)
