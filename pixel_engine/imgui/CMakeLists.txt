cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE MODULE_SOURCES "src/*.c" "src/*.cpp")
set(ENGINE_SOURCES ${ENGINE_SOURCES} ${MODULE_SOURCES} PARENT_SCOPE)
set(ENGINE_INCLUDES ${ENGINE_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)
add_library(epix_imgui STATIC ${MODULE_SOURCES})
target_include_directories(epix_imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(epix_imgui pixel_engine::app)
target_link_libraries(epix_imgui pixel_engine::window)
target_link_libraries(epix_imgui pixel_engine::render::ogl)
target_link_libraries(epix_imgui pixel_engine::render::vulkan)
target_link_libraries(epix_imgui imgui)

add_library(pixel_engine::imgui ALIAS epix_imgui)
add_library(epix::imgui ALIAS epix_imgui)