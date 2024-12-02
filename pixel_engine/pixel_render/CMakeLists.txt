cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE MODULE_SOURCES "src/*.c" "src/*.cpp")
set(ENGINE_SOURCES ${ENGINE_SOURCES} ${MODULE_SOURCES} PARENT_SCOPE)
set(ENGINE_INCLUDES ${ENGINE_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)
add_library(epix_pxrd STATIC ${MODULE_SOURCES})
target_include_directories(epix_pxrd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(epix_pxrd pixel_engine::app)
target_link_libraries(epix_pxrd pixel_engine::window)
target_link_libraries(epix_pxrd pixel_engine::render::vulkan)
target_link_libraries(epix_pxrd pixel_engine::render::font)

add_library(pixel_engine::render::pixel ALIAS epix_pxrd)
add_library(epix::render::pixel ALIAS epix_pxrd)