﻿cmake_minimum_required (VERSION 3.21)

get_filename_component(PROJNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(PROJNAME "example_${PROJNAME}")
project(${PROJNAME})

#add self files
file(GLOB_RECURSE SOURCE_FILES "include/*.h" "include/*.hpp" "src/*.c" "src/*.cpp")
add_executable (${PROJNAME} ${SOURCE_FILES})
target_include_directories(${PROJNAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

#third-party includes

#third-party libs
target_link_libraries (${PROJNAME} glfw)
target_link_libraries (${PROJNAME} Vulkan::Vulkan)
target_link_libraries (${PROJNAME} OpenGL::GL)
target_link_libraries (${PROJNAME} pixel_engine)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)