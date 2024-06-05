cmake_minimum_required(VERSION 3.21)

project(BSThreadPool)

set(CMAKE_CXX_STANDARD 17)

add_library(BSThreadPool STATIC
	src/BSThreadPool.cpp
)
target_include_directories(BSThreadPool PUBLIC include)