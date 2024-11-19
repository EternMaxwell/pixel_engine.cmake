cmake_minimum_required (VERSION 3.21)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("pixel_engine.cmake")

set(CMAKE_CXX_STANDARD 20)

if (MSVC)
  add_compile_options(/bigobj)
else ()
  add_compile_options(-Wa,-mbig-obj)
endif ()

#find third-party if possible
find_package (OpenGL REQUIRED)
find_package (Vulkan REQUIRED)

#sub directories
add_subdirectory (libs/entt)
add_subdirectory (libs/sparsepp)
add_subdirectory (libs/glad)
add_subdirectory (libs/glfw)
add_subdirectory (libs/spdlog)
add_subdirectory (libs/fmt)
add_subdirectory (libs/pfr)
add_subdirectory (libs/thread-pool)
add_subdirectory (libs/glm)
add_subdirectory (libs/freetype)
add_subdirectory (libs/vma)
add_subdirectory (libs/imgui)
add_subdirectory (libs/box2d)
add_subdirectory (pixel_engine)
add_subdirectory (examples)
