#
# Copyright (C) 2024 Patrick Rotsaert
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.19)

function(install_project_header HEADER BASE_DIR)
	file(REAL_PATH ${HEADER} HEADER_REALPATH)
	file(RELATIVE_PATH HEADER_RELPATH ${BASE_DIR} ${HEADER_REALPATH})
	get_filename_component(DIR ${HEADER_RELPATH} DIRECTORY)
	install(
		FILES ${HEADER}
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${DIR}
		COMPONENT ${COMPONENT_DEVELOPMENT}
	)
endfunction()

function(add_project_library TARGET)
	set(OPTIONS)
	set(ONE_VALUE_ARGS)
	set(MULTI_VALUE_ARGS
		SOURCES
		PUBLIC_HEADERS
		UNIT_TEST_SOURCES
		MOCK_SOURCES
		PRIVATE_DEFINITIONS
		PRIVATE_INCLUDE_DIRS
		PUBLIC_INCLUDE_DIRS
		PRIVATE_LIBRARIES
		PUBLIC_LIBRARIES
	)
	cmake_parse_arguments(P "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

	# Set the target output name
	set(TARGET_OUTPUT_NAME ${TARGET})

	if(WIN32)
		if(NOT BUILD_SHARED_LIBS)
			set(TARGET_OUTPUT_NAME ${TARGET_OUTPUT_NAME}-static)
		endif()
		set(TARGET_OUTPUT_NAME "${TARGET_OUTPUT_NAME}_${${PROJECT_NAME}_VERSION_MAJOR}_${${PROJECT_NAME}_VERSION_MINOR}")
	endif()

	# Create a list of compile and link targets
	set(COMPILE_TARGETS)
	set(LINK_TARGETS ${TARGET})

	# Add the library target
	if(P_UNIT_TEST_SOURCES AND ${PROJECT_NAME_UC}_TEST)
		add_library(${TARGET}_objects OBJECT ${P_SOURCES} ${P_PUBLIC_HEADERS})
		list(APPEND COMPILE_TARGETS ${TARGET}_objects)

		add_library(${TARGET} $<TARGET_OBJECTS:${TARGET}_objects>)
		list(APPEND COMPILE_TARGETS ${TARGET})

		add_executable(${TARGET}_unit_test ${P_UNIT_TEST_SOURCES} $<TARGET_OBJECTS:${TARGET}_objects>)

		if (P_MOCK_SOURCES)
			target_sources(${TARGET}_unit_test PRIVATE ${P_MOCK_SOURCES})
			target_link_libraries(${TARGET}_unit_test PRIVATE gmock)
		endif()

		list(APPEND COMPILE_TARGETS ${TARGET}_unit_test)

		target_link_libraries(${TARGET}_unit_test PRIVATE gtest_main)
		list(APPEND LINK_TARGETS ${TARGET}_unit_test)

		gtest_discover_tests(${TARGET}_unit_test)

		run_unit_test_on_build(${TARGET}_unit_test)
	else()
		add_library(${TARGET} ${P_SOURCES} ${P_PUBLIC_HEADERS})
		list(APPEND COMPILE_TARGETS ${TARGET})
	endif()

	# Add library alias
	add_library(${PROJECT_NAME}::${TARGET} ALIAS ${TARGET})

	# Hide all symbols by default
	set_target_properties(${TARGET} PROPERTIES
		CXX_VISIBILITY_PRESET hidden
		C_VISIBILITY_PRESET hidden
		VISIBILITY_INLINES_HIDDEN ON
	)

	foreach(TARGET ${COMPILE_TARGETS})
		# Specify the C++ standard
		target_compile_features(${TARGET} PRIVATE cxx_std_20)

		# PIC
		set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE True)

		# Set the compiler warning options
		target_compile_options(${TARGET} PRIVATE
			"$<$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>:$<BUILD_INTERFACE:-Wall;-Wextra;-pedantic;-Werror>>"
			"$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:$<BUILD_INTERFACE:/W4;/WX>>"
		)

		# Set compiler definitions
		if(P_PRIVATE_DEFINITIONS)
			target_compile_definitions(${TARGET} PRIVATE ${P_PRIVATE_DEFINITIONS})
		endif()

		# Set include dirs
		if(P_PRIVATE_INCLUDE_DIRS)
			target_include_directories(${TARGET} PRIVATE $<BUILD_INTERFACE:${P_PRIVATE_INCLUDE_DIRS}>)
		endif()
		if(P_PUBLIC_INCLUDE_DIRS)
			target_include_directories(${TARGET} PUBLIC $<BUILD_INTERFACE:${P_PUBLIC_INCLUDE_DIRS}>)
		endif()

		# Set the common include directory
		target_include_directories(${TARGET} PUBLIC
			$<BUILD_INTERFACE:${PROJECT_SRC_DIR}>
			$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
		)

		# Link libraries
		if(P_PUBLIC_LIBRARIES)
			target_link_libraries(${TARGET} PUBLIC ${P_PUBLIC_LIBRARIES})
		endif()
		if(P_PRIVATE_LIBRARIES)
			target_link_libraries(${TARGET} PRIVATE ${P_PRIVATE_LIBRARIES})
		endif()
	endforeach()

	foreach(TARGET ${LINK_TARGETS})
		set_target_properties(${TARGET} PROPERTIES DEBUG_POSTFIX "-d")
	endforeach()

	# Set other target properties
	set_target_properties(${TARGET} PROPERTIES
		VERSION ${${PROJECT_NAME}_VERSION}
		SOVERSION ${${PROJECT_NAME}_VERSION_MAJOR}
		OUTPUT_NAME "${TARGET_OUTPUT_NAME}"
	)

	if(BUILD_SHARED_LIBS)
		target_compile_definitions(${TARGET}
			PUBLIC BITCASK_SHARED
			PRIVATE BITCASK_SHARED_EXPORTS
		)
	endif()

	if(${PROJECT_NAME_UC}_INSTALL)
		# Install the library
		install(TARGETS ${TARGET}
			EXPORT ${PROJECT_NAME}_Targets
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			        COMPONENT ${COMPONENT_RUNTIME}
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			        COMPONENT ${COMPONENT_RUNTIME}
			        NAMELINK_COMPONENT ${COMPONENT_DEVELOPMENT}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			        COMPONENT ${COMPONENT_DEVELOPMENT}
		)

		# Install public header files
		foreach(HEADER ${P_PUBLIC_HEADERS})
			install_project_header(${HEADER} ${PROJECT_SRC_DIR})
		endforeach()
	endif()
endfunction()

add_subdirectory(bitcask)
add_subdirectory(examples)
