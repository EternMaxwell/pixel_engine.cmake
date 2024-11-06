cmake_minimum_required(VERSION 3.21)

project(sparsepp)

add_library(sparsepp INTERFACE)
target_include_directories(sparsepp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(sparsepp PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)