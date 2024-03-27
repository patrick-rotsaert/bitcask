//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "bitcask/spdlog_logger.h"
#include "bitcask/logging.h"

#include <spdlog/spdlog.h>

namespace bitcask {

namespace {

spdlog::level::level_enum spdlog_level(log_level in)
{
	switch (in)
	{
	case log_level::trace:
		return spdlog::level::trace;
	case log_level::debug:
		return spdlog::level::debug;
	case log_level::info:
		return spdlog::level::info;
	case log_level::warn:
		return spdlog::level::warn;
	case log_level::err:
		return spdlog::level::err;
	case log_level::critical:
		return spdlog::level::critical;
	case log_level::off:
		return spdlog::level::off;
	}
	return spdlog::level::off;
}

} // namespace

spdlog_logger::spdlog_logger()
{
}

spdlog_logger::~spdlog_logger()
{
}

void spdlog_logger::log_message(const std::chrono::system_clock::time_point& time,
                                const boost::source_location&                location,
                                log_level                                    level,
                                const std::string_view&                      message)
{
	auto logger = spdlog::default_logger_raw();
	if (logger)
	{
		logger->log(time,
		            spdlog::source_loc{ location.file_name(), static_cast<int>(location.line()), location.function_name() },
		            spdlog_level(level),
		            spdlog::string_view_t{ message });
	}
}

} // namespace bitcask
