cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE MODULE_SOURCES "src/*.c" "src/*.cpp")
set(ENGINE_SOURCES ${ENGINE_SOURCES} ${MODULE_SOURCES} PARENT_SCOPE)
set(ENGINE_INCLUDES ${ENGINE_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)
add_library(epix_sprite STATIC ${MODULE_SOURCES})
target_include_directories(epix_sprite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(epix_sprite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/stb)

target_link_libraries(epix_sprite freetype)
target_link_libraries(epix_sprite Vulkan::Vulkan)
target_link_libraries(epix_sprite glfw)
target_link_libraries(epix_sprite pixel_engine::app)
target_link_libraries(epix_sprite pixel_engine::window)
target_link_libraries(epix_sprite pixel_engine::render::vulkan)
target_link_libraries(epix_sprite pixel_engine::render::font)

add_library(pixel_engine::render::sprite ALIAS epix_sprite)
add_library(epix::render::sprite ALIAS epix_sprite)