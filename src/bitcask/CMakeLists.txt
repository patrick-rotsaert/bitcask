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
	# If these libraries are not pre-installed, then they will be fetched from github (see src/bitcask/CMakeLists.txt)
	# and then their install targets would be included, which is probably not what you want.
	find_package(fmt REQUIRED)
	if(${PROJECT_NAME_UC}_USE_SPDLOG)
		find_package(spdlog REQUIRED)
	endif()
endif()

# FIXME: improve this
if(TARGET Boost::system AND TARGET Boost::filesystem AND TARGET Boost::date_time)
	message("Boost targets already defined")
else()
	find_package(Boost REQUIRED COMPONENTS system filesystem date_time)
endif()

include(FetchContent)

# {fmtlib}
# If this project is included as source, the fmt::fmt target may already be defined.
if(NOT TARGET fmt::fmt)
	# Prefer the system package
	find_package(fmt QUIET)
	if(NOT fmt_FOUND)
		set(FMT_INSTALL "${${PROJECT_NAME_UC}_INSTALL}")
		FetchContent_Declare(fmt
		  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
		  GIT_TAG 10.0.0
		)
		FetchContent_MakeAvailable(fmt)
	endif()
endif()

# spdlog
if(${PROJECT_NAME_UC}_USE_SPDLOG)
	# If this project is included as source, the spdlog::spdlog target may already be defined.
	if(NOT TARGET spdlog::spdlog)
		# Prefer the system package
		find_package(spdlog QUIET)
		if(NOT spdlog_FOUND)
			set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
			set(SPDLOG_FMT_EXTERNAL ON)
			set(SPDLOG_INSTALL "${${PROJECT_NAME_UC}_INSTALL}")
			FetchContent_Declare(spdlog
			  GIT_REPOSITORY https://github.com/gabime/spdlog.git
			  GIT_TAG v1.13.0
			)
			FetchContent_MakeAvailable(spdlog)
		endif()
	endif()
	list(APPEND EXTRA_PUBLIC_LIBRARIES spdlog::spdlog)
	list(APPEND EXTRA_SOURCES spdlog_logger.cpp spdlog_logger.h)
	if(${PROJECT_NAME_UC}_INSTALL)
		list(APPEND EXTRA_PUBLIC_HEADERS spdlog_logger.h)
	endif()
endif()

# Threads
if(${PROJECT_NAME_UC}_THREAD_SAFE)
	find_package(Threads REQUIRED)
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
		Boost::system
		Boost::filesystem
		Boost::date_time
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
