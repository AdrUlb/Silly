project(Silly)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SILLY_ROOT ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
		${SILLY_ROOT}/Source/*.c
		${SILLY_ROOT}/Source/*.h
		${SILLY_ROOT}/Source/*.cpp
		${SILLY_ROOT}/Source/*.hpp
		${SILLY_ROOT}/Source/*.tpp
		${SILLY_ROOT}/Include/*.h
		${SILLY_ROOT}/Include/*.hpp
)

function(silly_library_define target_name)
	cmake_parse_arguments(ARG "SHARED STATIC" "" "" ${ARGN})

	message(WARNING ${SOURCES})
	set(LIB_TYPE "")
	if (ARG_SHARED)
		set(LIB_TYPE SHARED)
	elseif (ARG_STATIC)
		set(LIB_TYPE STATIC)
	endif ()

	add_library(${target_name} ${LIB_TYPE} ${SOURCES})
	target_include_directories(${target_name}
			PRIVATE ${SILLY_ROOT}/Source
			PUBLIC ${SILLY_ROOT}/Include
	)

	target_compile_options(${target_name} PRIVATE -Wall -Wextra)
endfunction()
