cmake_minimum_required (VERSION 3.21)

project (pixel_engine VERSION 0.1.0)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

#set_target_properties(pixel_engine PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} PUBLIC_HEADER ${ENGINE_HEADERS})

#include glfw vulkan ... etc
#target_include_directories(pixel_engine PUBLIC )

#link glfw vulkan ... etc
target_link_libraries (pixel_engine glfw)
target_link_libraries (pixel_engine OpenGL::GL)
target_link_libraries (pixel_engine Vulkan::Vulkan)
target_link_libraries (pixel_engine EnTT::EnTT)
target_link_libraries (pixel_engine spdlog::spdlog)
target_link_libraries (pixel_engine fmt::fmt)
target_link_libraries (pixel_engine Boost::pfr)
target_link_libraries (pixel_engine BSThreadPool)