cmake_minimum_required(VERSION 3.0)

project(fm LANGUAGES C)

add_executable(fm main.c array.c)
set(CMAKE_BUILD_TYPE Debug)
install(TARGETS fm RUNTIME DESTINATION bin)

