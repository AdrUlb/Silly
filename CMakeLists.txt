cmake_minimum_required(VERSION 4.2)
project(Silly)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS Source/*.cpp Source/*.hpp Source/*.tpp Source/*.c Source/*.h Include/*.hpp Include/*.tpp Include/*h)

add_library(Silly STATIC ${SOURCES})
target_include_directories(Silly PUBLIC Include)
target_compile_options(Silly PRIVATE -Wall -Wextra -nostdlib -ffreestanding)
