//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "bitcask/log_level.h"
#include "bitcask/api.h"

#include <boost/assert/source_location.hpp>

#include <chrono>
#include <string_view>

namespace bitcask {

class BITCASK_EXPORT ilogger
{
public:
	ilogger();
	virtual ~ilogger();

	virtual void log_message(const std::chrono::system_clock::time_point& time,
	                         const boost::source_location&                location,
	                         log_level                                    level,
	                         const std::string_view&                      message) = 0;
};

} // namespace bitcask
