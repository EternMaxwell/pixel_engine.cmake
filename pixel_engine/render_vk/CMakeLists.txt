cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine_render_vk STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})
target_include_directories(pixel_engine_render_vk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixel_engine_render_vk spdlog::spdlog)
target_link_libraries(pixel_engine_render_vk Vulkan::Vulkan)
target_link_libraries(pixel_engine_render_vk VulkanMemoryAllocator)
target_link_libraries(pixel_engine_render_vk glm::glm)
target_link_libraries(pixel_engine_render_vk glfw)
target_link_libraries(pixel_engine_render_vk pixel_engine::app)
target_link_libraries(pixel_engine_render_vk pixel_engine::window)
target_link_libraries(pixel_engine_render_vk fmt)

add_library(pixel_engine::render::vulkan ALIAS pixel_engine_render_vk)