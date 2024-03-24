#
# Copyright (C) 2024 Patrick Rotsaert
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#

include_guard(GLOBAL)

# if(    TARGET Boost::system
#    AND TARGET Boost::filesystem
#    AND TARGET Boost::date_time
#    AND TARGET Boost::thread
#    )
# 	message(TRACE "Boost targets already defined")
# else()
# 	find_package(Boost REQUIRED COMPONENTS system filesystem date_time thread)
# endif()

include(FetchContent)

# {fmtlib}
# If this project is included as a subdirectory, the fmt::fmt target may already be defined.
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

# # spdlog
# if(${PROJECT_NAME_UC}_USE_SPDLOG)
# 	# If this project is included as a subdirectory, the spdlog::spdlog target may already be defined.
# 	if(NOT TARGET spdlog::spdlog)
# 		# Prefer the system package
# 		find_package(spdlog QUIET)
# 		if(NOT spdlog_FOUND)
# 			set(SPDLOG_FMT_EXTERNAL ON)
# 			set(SPDLOG_INSTALL "${${PROJECT_NAME_UC}_INSTALL}")
# 			FetchContent_Declare(spdlog
# 			  GIT_REPOSITORY https://github.com/gabime/spdlog.git
# 			  GIT_TAG v1.12.0
# 			)
# 			FetchContent_MakeAvailable(spdlog)
# 		endif()
# 	endif()
# endif()
