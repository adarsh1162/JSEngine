cmake_minimum_required(VERSION 3.10)
project(JSEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add all source files here
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Define the executable
add_executable(JSEngine ${SOURCES})
