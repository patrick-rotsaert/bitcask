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

include(cmake/common/vars.cmake)

option(${PROJECT_NAME_UC}_BUILD_EXAMPLES "Build the examples." "${${PROJECT_NAME_UC}_IS_TOP_LEVEL}")
option(${PROJECT_NAME_UC}_TEST "Build the tests." "${${PROJECT_NAME_UC}_IS_TOP_LEVEL}")
option(${PROJECT_NAME_UC}_RUN_UNIT_TESTS_ON_BUILD "Run the unit tests during build." "${${PROJECT_NAME_UC}_IS_TOP_LEVEL}")
option(${PROJECT_NAME_UC}_INSTALL "Include install rules for ${PROJECT_NAME}." "${${PROJECT_NAME_UC}_IS_TOP_LEVEL}")
option(${PROJECT_NAME_UC}_CPACK "Include cpack rules for ${PROJECT_NAME}. Implies ${PROJECT_NAME_UC}_INSTALL." "${${PROJECT_NAME_UC}_IS_TOP_LEVEL}")

option(${PROJECT_NAME_UC}_USE_SPDLOG "Use spdlog as the default logger in the ${PROJECT_NAME} library" "${${PROJECT_NAME_UC}_IS_TOP_LEVEL}")
option(${PROJECT_NAME_UC}_THREAD_SAFE "Compile with locking code" true)

if(${PROJECT_NAME_UC}_CPACK)
	set(${PROJECT_NAME_UC}_INSTALL true)
endif()

# Let ${PROJECT_NAME_UC}_SHARED_LIBS override BUILD_SHARED_LIBS
if(DEFINED ${PROJECT_NAME_UC}_SHARED_LIBS)
    set(BUILD_SHARED_LIBS "${${PROJECT_NAME_UC}_SHARED_LIBS}")
endif()

include(cmake/common/project.cmake)
include(cmake/common/paths.cmake)

add_subdirectory(src)

if(${PROJECT_NAME_UC}_INSTALL)
	add_subdirectory(install)
	if(${PROJECT_NAME_UC}_CPACK)
		add_subdirectory(cpack)
	endif()
endif()
