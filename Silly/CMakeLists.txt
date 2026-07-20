file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS Source/*.cpp Source/*.hpp Source/*.tpp Source/*.c Source/*.h Include/*.hpp Include/*.tpp Include/*h)

add_library(Silly STATIC ${SOURCES})
target_include_directories(Silly PUBLIC Include)
target_compile_options(Silly PRIVATE -Wall -Wextra)
target_compile_options(Silly PRIVATE -nostdlib -ffreestanding)
target_compile_definitions(Silly PRIVATE -DKERNEL)
