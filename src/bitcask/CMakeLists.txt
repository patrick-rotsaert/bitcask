#
# Copyright (C) 2024 Patrick Rotsaert
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.19)

set(EXTRA_SOURCES)
set(EXTRA_PUBLIC_LIBRARIES)
set(EXTRA_PUBLIC_HEADERS)

if(${PROJECT_NAME_UC}_INSTALL)
	# When install rules are included, then the fmt and spdlog packages must be pre-installed.
	# If not, then they will be fetched from github (see included fmtlib.cmake and spdlog.cmake)
	# and then their install targets would be included, which is probably not what you want.
	project_find_package(fmt REQUIRED)
	if(${PROJECT_NAME_UC}_USE_SPDLOG)
		project_find_package(spdlog REQUIRED)
	endif()
endif()

# {fmtlib}
include(${PROJECT_SOURCE_DIR}/cmake/common/fmtlib.cmake)

# spdlog
if(${PROJECT_NAME_UC}_USE_SPDLOG)
	include(${PROJECT_SOURCE_DIR}/cmake/common/spdlog.cmake)
	list(APPEND EXTRA_PUBLIC_LIBRARIES spdlog::spdlog)
	list(APPEND EXTRA_SOURCES spdlog_logger.cpp spdlog_logger.h)
	if(${PROJECT_NAME_UC}_INSTALL)
		list(APPEND EXTRA_PUBLIC_HEADERS spdlog_logger.h)
	endif()
endif()

# Threads
if(${PROJECT_NAME_UC}_THREAD_SAFE)
	project_find_package(Threads REQUIRED)
	list(APPEND EXTRA_PUBLIC_LIBRARIES Threads::Threads)
endif()

add_project_library(${PROJECT_NAME}
	SOURCES
		bitcask.cpp
		datadir.cpp
		datadir.h
		datafile.cpp
		datafile.h
		hintfile.cpp
		hintfile.h
		keydir.cpp
		keydir.h
		basictypes.h
		locktypes.hpp
		lockfile.cpp
		lockfile.h
		lockfile_impl_posix.hpp
		crc32.cpp
		crc32.h
		file.cpp
		file.h
		hton.h
		ilogger.cpp
		logging.cpp
		formatters.h
		conversions.cpp
		version.cpp
		version.h.in
		config.h.in
		${EXTRA_SOURCES}

	PUBLIC_HEADERS
		api.h
		bitcask.h
		conversions.h
		ilogger.h
		logging.h
		log_level.h
		${EXTRA_PUBLIC_HEADERS}

	PUBLIC_LIBRARIES
		fmt::fmt
		${EXTRA_PUBLIC_LIBRARIES}

	UNIT_TEST_SOURCES
		test/unit/test_conversions.cpp

	PUBLIC_INCLUDE_DIRS
		${CMAKE_CURRENT_BINARY_DIR}/.. # for configured headers, see below
)

#
# Build time logging levels

# Values are as in log_level.h
set(LOGGING_LEVELS trace debug info warn err critical off)

if(${PROJECT_NAME_UC}_DEBUG_BUILD)
	set(DEFAULT_LOGGING_LEVEL trace)
else()
	set(DEFAULT_LOGGING_LEVEL warn)
endif()

# Logging verbosity of the library
set(${PROJECT_NAME_UC}_LOGGING_LEVEL ${DEFAULT_LOGGING_LEVEL} CACHE STRING "Logging verbosity of the ${PROJECT_NAME} library")
set_property(CACHE ${PROJECT_NAME_UC}_LOGGING_LEVEL PROPERTY STRINGS ${LOGGING_LEVELS})

configure_file(
	version.h.in
	${CMAKE_CURRENT_BINARY_DIR}/version.h
)

configure_file(
	config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/config.h
)

if(${PROJECT_NAME_UC}_INSTALL)
	install_project_header(${CMAKE_CURRENT_BINARY_DIR}/version.h ${CMAKE_CURRENT_BINARY_DIR}/..)
	install_project_header(${CMAKE_CURRENT_BINARY_DIR}/config.h ${CMAKE_CURRENT_BINARY_DIR}/..)
endif()
