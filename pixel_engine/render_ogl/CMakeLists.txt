cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_render_ogl STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine_render_ogl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixel_engine_render_ogl spdlog::spdlog)
target_link_libraries(pixel_engine_render_ogl Vulkan::Vulkan)
target_link_libraries(pixel_engine_render_ogl VulkanMemoryAllocator)
target_link_libraries(pixel_engine_render_ogl glm::glm)
target_link_libraries(pixel_engine_render_ogl glfw)
target_link_libraries(pixel_engine_render_ogl pixel_engine::app)
target_link_libraries(pixel_engine_render_ogl pixel_engine::window)
target_link_libraries(pixel_engine_render_ogl fmt)

add_library(pixel_engine::render::ogl ALIAS pixel_engine_render_ogl)

add_subdirectory(test)
