file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS Source/*.cpp Source/*.hpp Source/*.c Source/*.h)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

add_executable(Test ${SOURCES})
target_compile_options(Test PRIVATE -Wall -Wextra)
target_link_libraries(Test PRIVATE Silly SDL3::SDL3)
