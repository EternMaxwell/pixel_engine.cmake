# CMakeList.txt: pixel_engine.cmake 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.21)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(PROJNAME "pixel_engine.cmake")
project (${PROJNAME})

# 包含子项目。

# 将源代码添加到此项目的可执行文件。
file(GLOB_RECURSE SOURCE_FILES "include/*.h" "include/*.hpp" "src/*.c" "src/*.cpp")
add_executable (${PROJNAME} ${SOURCE_FILES})
target_include_directories(${PROJNAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)

#prepare third-party
set(VULKAN_SDK_DIR "C:/VulkanSDK/1.3.280.0")
set(VULKAN_LIB ${VULKAN_SDK_DIR}/Lib/vulkan-1.lib)

file(GLOB_RECURSE VULKAN_LIBS ${VULKAN_LIB_DIR} vulkan-1.lib glslang.lib)

#find third-party if possible
find_package(OpenGL REQUIRED)

#third-party includes
target_include_directories(${PROJNAME} PUBLIC ${CMAKE_SOURCE_DIR}/lib/glfw/include)
target_include_directories(${PROJNAME} PUBLIC ${VULKAN_SDK_DIR}/Include)
target_include_directories(${PROJNAME} PUBLIC ${OPENGL_INCLUDE_DIR})

#third-party libs
target_link_libraries (${PROJNAME} ${CMAKE_SOURCE_DIR}/lib/glfw/lib-vc2022/glfw3.lib)
target_link_libraries (${PROJNAME} OpenGL::GL)
target_link_libraries (${PROJNAME} ${VULKAN_LIB})

#sub directories
add_subdirectory (pixel_engine)
target_link_libraries (${PROJNAME} pixel_engine)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJNAME} PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
