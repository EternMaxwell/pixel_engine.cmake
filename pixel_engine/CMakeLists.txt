﻿cmake_minimum_required (VERSION 3.21)

add_subdirectory(app)
add_subdirectory(app_v2)
add_subdirectory(window)
add_subdirectory(input)
add_subdirectory(render_ogl)
add_subdirectory(vulkan_graph)
add_subdirectory(render_vk)
add_subdirectory(imgui)
add_subdirectory(debug_draw)
add_subdirectory(font)
add_subdirectory(sprite)
add_subdirectory(pixel_render)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

#set_target_properties(pixel_engine PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} PUBLIC_HEADER ${ENGINE_HEADERS})

#include glfw vulkan ... etc
#target_include_directories(pixel_engine PUBLIC )
target_include_directories(pixel_engine PUBLIC ../libs/stb)

#link glfw vulkan ... etc
target_link_libraries (pixel_engine PUBLIC glfw)
target_link_libraries (pixel_engine PUBLIC OpenGL::GL)
target_link_libraries (pixel_engine PUBLIC glad)
target_link_libraries (pixel_engine PUBLIC Vulkan::Vulkan)
target_link_libraries (pixel_engine PUBLIC EnTT::EnTT)
target_link_libraries (pixel_engine PUBLIC spdlog::spdlog)
target_link_libraries (pixel_engine PUBLIC Boost::pfr)
target_link_libraries (pixel_engine PUBLIC BSThreadPool)
target_link_libraries (pixel_engine PUBLIC glm::glm)
target_link_libraries (pixel_engine PUBLIC freetype)
target_link_libraries (pixel_engine PUBLIC VulkanMemoryAllocator)
target_link_libraries (pixel_engine PUBLIC pixel_engine::app)
target_link_libraries (pixel_engine PUBLIC pixel_engine::window)
target_link_libraries (pixel_engine PUBLIC pixel_engine::input)
target_link_libraries (pixel_engine PUBLIC pixel_engine::render::vulkan)
target_link_libraries (pixel_engine PUBLIC pixel_engine::render::debug)
target_link_libraries (pixel_engine PUBLIC pixel_engine::render::font)
target_link_libraries (pixel_engine PUBLIC pixel_engine::render::sprite)
target_link_libraries (pixel_engine PUBLIC pixel_engine::render::pixel)