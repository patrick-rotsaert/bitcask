cmake_minimum_required(VERSION 3.22)
project(bitcask CXX)

include(deps.cmake)

function(add_cxx_executable TARGET)
	add_executable(${TARGET} ${ARGN})
	target_compile_features(${TARGET} PRIVATE cxx_std_20)
	target_compile_options(${TARGET} PRIVATE
		"$<$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>:$<BUILD_INTERFACE:-Wall;-Wextra;-pedantic;-Werror>>"
		"$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:$<BUILD_INTERFACE:/W4;/WX>>"
	)
endfunction()

add_cxx_executable(bitcask
	bitcask.cpp
	bitcask.h
	datadir.cpp
	datadir.h
	datafile.cpp
	datafile.h
	hintfile.cpp
	hintfile.h
	keydir.cpp
	keydir.h
	basic_types.h
	crc32.cpp
	crc32.h
	file.cpp
	file.h
	hton.h

	main.cpp
)

target_link_libraries(bitcask PRIVATE fmt::fmt)
