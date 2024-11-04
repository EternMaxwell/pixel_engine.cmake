cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_app_v2 STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine_app_v2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixel_engine_app_v2 EnTT::EnTT)
target_link_libraries(pixel_engine_app_v2 BSThreadPool)
target_link_libraries(pixel_engine_app_v2 spdlog::spdlog)
target_link_libraries(pixel_engine_app_v2 sparsepp)

add_library(pixel_engine::app2 ALIAS pixel_engine_app_v2)

add_subdirectory(tests)
