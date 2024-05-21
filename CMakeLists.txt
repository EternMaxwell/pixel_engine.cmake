# CMakeList.txt: pixel_engine.cmake 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.21)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("pixel_engine.cmake")

set(CMAKE_CXX_STANDARD 20)

#prepare third-party
set(VULKAN_SDK_DIR "C:/VulkanSDK/1.3.280.0")
set(VULKAN_LIB ${VULKAN_SDK_DIR}/Lib/vulkan-1.lib)

file(GLOB_RECURSE VULKAN_LIBS ${VULKAN_LIB_DIR} vulkan-1.lib glslang.lib)

#find third-party if possible
find_package(OpenGL REQUIRED)

#sub directories
add_subdirectory (lib/entt)
add_subdirectory (pixel_engine)
add_subdirectory (examples)
