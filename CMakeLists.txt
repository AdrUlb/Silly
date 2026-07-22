cmake_minimum_required(VERSION 3.25)
project(Silly)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS Source/*.c Source/*.h Source/*.cpp Source/*.hpp Source/*.tpp Include/*.h Include/*.hpp)

add_library(Silly STATIC ${SOURCES})
target_include_directories(Silly PRIVATE Source PUBLIC Include)
target_compile_options(Silly PRIVATE -Wall -Wextra -nostdlib -ffreestanding)
