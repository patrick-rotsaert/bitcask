#
# Copyright (C) 2024 Patrick Rotsaert
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.19)
project(bitcask VERSION 0.1.0 LANGUAGES CXX)

# Note to self: include this as early as possible
include(GNUInstallDirs)

enable_testing()

string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UC)
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" ${PROJECT_NAME_UC}_IS_TOP_LEVEL)

option(${PROJECT_NAME_UC}_BUILD_EXAMPLES "Build the examples" "${${PROJECT_NAME_UC}_IS_TOP_LEVEL}")
option(${PROJECT_NAME_UC}_TEST "Build the tests" "${${PROJECT_NAME_UC}_IS_TOP_LEVEL}")
option(${PROJECT_NAME_UC}_RUN_UNIT_TESTS_ON_BUILD "Run the unit tests during build" "${${PROJECT_NAME_UC}_IS_TOP_LEVEL}")
option(${PROJECT_NAME_UC}_INSTALL "Include install rules for ${PROJECT_NAME}" "${${PROJECT_NAME_UC}_IS_TOP_LEVEL}")
option(${PROJECT_NAME_UC}_CPACK "Include cpack rules for ${PROJECT_NAME}" "${${PROJECT_NAME_UC}_IS_TOP_LEVEL}")

option(${PROJECT_NAME_UC}_USE_SPDLOG "Use spdlog as the default logger in the ${PROJECT_NAME} library" "${${PROJECT_NAME_UC}_IS_TOP_LEVEL}")
option(${PROJECT_NAME_UC}_THREAD_SAFE "Compile with locking code" true)

include(cmake/buildvars.cmake)
include(cmake/paths.cmake)
include(cmake/version.cmake)
include(cmake/gtest.cmake)

# Let ${PROJECT_NAME}_SHARED_LIBS override BUILD_SHARED_LIBS
if(DEFINED ${PROJECT_NAME_UC}_SHARED_LIBS)
    set(BUILD_SHARED_LIBS "${${PROJECT_NAME_UC}_SHARED_LIBS}")
endif()

# Set the component names
# These are prefixed with ${PROJECT_NAME} to avoid name clashes with component names
# of projects that include this library as source.
set(COMPONENT_RUNTIME ${PROJECT_NAME}_runtime)
set(COMPONENT_DEVELOPMENT ${PROJECT_NAME}_development)

add_subdirectory(src)

if(${PROJECT_NAME_UC}_INSTALL)
	add_subdirectory(install)
	if(${PROJECT_NAME_UC}_CPACK)
		add_subdirectory(cpack)
	endif()
endif()
