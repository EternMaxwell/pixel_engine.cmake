cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE MODULE_SOURCES "src/*.c" "src/*.cpp")
set(ENGINE_SOURCES ${ENGINE_SOURCES} ${MODULE_SOURCES} PARENT_SCOPE)
set(ENGINE_INCLUDES ${ENGINE_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)
add_library(epix_input STATIC ${MODULE_SOURCES})
target_include_directories(epix_input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(epix_input pixel_engine::app)
target_link_libraries(epix_input glfw)
target_link_libraries(epix_input OpenGL::GL)
target_link_libraries(epix_input glad)
target_link_libraries(epix_input Vulkan::Vulkan)
target_link_libraries(epix_input pixel_engine::window)

add_library(pixel_engine::input ALIAS epix_input)
add_library(epix::input ALIAS epix_input)