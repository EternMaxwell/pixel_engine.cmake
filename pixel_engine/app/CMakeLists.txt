cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(epix_app STATIC ${ENGINE_SOURCES})
target_include_directories(epix_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(epix_app EnTT::EnTT)
target_link_libraries(epix_app BSThreadPool)
target_link_libraries(epix_app spdlog::spdlog)
target_link_libraries(epix_app sparsepp)

add_library(pixel_engine::app ALIAS epix_app)

add_subdirectory(tests)
