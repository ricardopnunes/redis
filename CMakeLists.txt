cmake_minimum_required(VERSION 3.20)
project(Redis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/Sanitizers.cmake)
include(cmake/ConfigWarnings.cmake)

add_subdirectory(src)
add_subdirectory(tests)
