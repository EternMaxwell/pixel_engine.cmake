cmake_minimum_required (VERSION 3.21)

project (pixel_engine VERSION 0.1.0)

file(GLOB_RECURSE ENGINE_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/*.c" "src/*.cpp")
add_library(pixel_engine STATIC ${ENGINE_SOURCES})
target_include_directories(pixel_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

#set_target_properties(pixel_engine PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} PUBLIC_HEADER ${ENGINE_HEADERS})

#include glfw vulkan ... etc
target_include_directories(pixel_engine PUBLIC ${CMAKE_SOURCE_DIR}/lib/glfw/include)
target_include_directories(pixel_engine PUBLIC ${VULKAN_SDK_DIR}/Include)
target_include_directories(pixel_engine PUBLIC ${OPENGL_INCLUDE_DIR})

#link glfw vulkan ... etc
target_link_libraries (pixel_engine ${CMAKE_SOURCE_DIR}/lib/glfw/lib-vc2022/glfw3.lib)
target_link_libraries (pixel_engine OpenGL::GL)

#install(TARGETS ${PROJECT_NAME} DESTINATION lib/${PROJECT_NAME})
#install(FILES ${ENGINE_HEADERS} DESTINATION include/${PROJECT_NAME})