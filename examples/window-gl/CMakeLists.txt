cmake_minimum_required (VERSION 3.21)

get_filename_component(PROJNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(PROJNAME "example_${PROJNAME}")
project(${PROJNAME})

#add self files
file(GLOB_RECURSE SOURCE_FILES "include/*.h" "include/*.hpp" "src/*.c" "src/*.cpp")
add_executable (${PROJNAME} ${SOURCE_FILES})
target_include_directories(${PROJNAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)

#third-party includes
target_include_directories(${PROJNAME} PUBLIC ${CMAKE_SOURCE_DIR}/lib/glfw/include)
target_include_directories(${PROJNAME} PUBLIC ${VULKAN_SDK_DIR}/Include)
target_include_directories(${PROJNAME} PUBLIC ${OPENGL_INCLUDE_DIR})

#third-party libs
target_link_libraries (${PROJNAME} ${CMAKE_SOURCE_DIR}/lib/glfw/lib-vc2022/glfw3.lib)
target_link_libraries (${PROJNAME} OpenGL::GL)
target_link_libraries (${PROJNAME} ${VULKAN_LIB})
target_link_libraries (${PROJNAME} pixel_engine)