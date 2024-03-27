//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "bitcask/ilogger.h"
#include "bitcask/api.h"
#include "bitcask/config.h"

#include <fmt/format.h>
#include <boost/assert/source_location.hpp>

#include <memory>
#include <chrono>

namespace bitcask {

class BITCASK_EXPORT logging
{
public:
	static std::unique_ptr<ilogger> logger;

	// Not thread safe
	static void set_logger(std::unique_ptr<ilogger>&& logger);
};

} // namespace bitcask

// Macro for logging using a fmtlib format string
#define bclog(lvl, ...)                                                                                                                    \
	do                                                                                                                                     \
	{                                                                                                                                      \
		auto& logger = ::bitcask::logging::logger;                                                                                         \
		if (logger)                                                                                                                        \
		{                                                                                                                                  \
			logger->log_message(                                                                                                           \
			    std::chrono::system_clock::now(), BOOST_CURRENT_LOCATION, ::bitcask::log_level::lvl, fmt::format(__VA_ARGS__));            \
		}                                                                                                                                  \
	} while (false)

// Macros for eliding logging code at compile time
#undef BITCASK_MIN_LOG
#define BITCASK_MIN_LOG(minlvl, lvl, ...)                                                                                                  \
	do                                                                                                                                     \
	{                                                                                                                                      \
		if constexpr (::bitcask::log_level::lvl >= ::bitcask::log_level::minlvl)                                                           \
		{                                                                                                                                  \
			bclog(lvl, __VA_ARGS__);                                                                                                       \
		}                                                                                                                                  \
	} while (false)

#undef BITCASK_LOG
#define BITCASK_LOG(lvl, ...) BITCASK_MIN_LOG(BITCASK_LOGGING_LEVEL, lvl, __VA_ARGS__)
