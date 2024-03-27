//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "bitcask/logging.h"
#include "bitcask/config.h"
#ifdef BITCASK_USE_SPDLOG
#include "bitcask/spdlog_logger.h"
#endif

namespace bitcask {

#ifdef BITCASK_USE_SPDLOG
std::unique_ptr<ilogger> logging::logger = std::make_unique<spdlog_logger>();
#else
std::unique_ptr<ilogger> logging::logger{};
#endif

void logging::set_logger(std::unique_ptr<ilogger>&& logger)
{
	logging::logger = std::move(logger);
}

} // namespace bitcask
