//
// Copyright (C) 2023 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "bitcask/api.h"
#include "bitcask/config.h"

#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/throw_exception.hpp>

#include <type_traits>
#include <charconv>
#include <stdexcept>
#include <system_error>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <string_view>

namespace bitcask {

using time_point  = std::chrono::system_clock::time_point;
using date        = std::chrono::year_month_day;
using time_of_day = std::chrono::hh_mm_ss<std::chrono::microseconds>;

template<typename T, class = typename std::enable_if<std::is_scalar_v<T>>::type>
inline void string_to_number(const char* first, const char* last, T& out)
{
	std::from_chars_result res = std::from_chars(first, last, out);
	if (res.ec != std::errc{})
	{
		BOOST_THROW_EXCEPTION(std::system_error{ std::error_code(static_cast<int>(res.ec), std::generic_category()) });
	}
	else if (res.ptr != last)
	{
		std::ostringstream msg;
		msg << "conversion incomplete, remaining input is " << std::quoted(std::string_view{ res.ptr, last });
		BOOST_THROW_EXCEPTION(std::invalid_argument{ msg.str() });
	}
}

template<typename T, class = typename std::enable_if<std::is_scalar_v<T>>::type>
inline void string_to_number(std::string_view in, T& out)
{
	string_to_number(in.data(), in.data() + in.length(), out);
}

template<typename T, class = typename std::enable_if<std::is_scalar_v<T>>::type>
inline T string_to_number(std::string_view in)
{
	T out{};
	string_to_number(in.data(), in.data() + in.length(), out);
	return out;
}

// Remark: The time_point resolution is intentionally limited to microseconds
// to avoid different behaviour depending on the platform.

void BITCASK_EXPORT       string_to_time_point(std::string_view in, time_point& out);
time_point BITCASK_EXPORT string_to_time_point(std::string_view in);

void BITCASK_EXPORT string_to_date(std::string_view in, date& out);
date BITCASK_EXPORT string_to_date(std::string_view in);

void BITCASK_EXPORT        string_to_time_of_day(std::string_view in, time_of_day& out);
time_of_day BITCASK_EXPORT string_to_time_of_day(std::string_view in);

void BITCASK_EXPORT        time_point_to_string(const time_point& in, std::string& out, const char date_time_separator = 'T');
std::string BITCASK_EXPORT time_point_to_string(const time_point& in, const char date_time_separator = 'T');

void BITCASK_EXPORT        date_to_string(const date& in, std::string& out);
std::string BITCASK_EXPORT date_to_string(const date& in);

void BITCASK_EXPORT        time_of_day_to_string(const time_of_day& in, std::string& out);
std::string BITCASK_EXPORT time_of_day_to_string(const time_of_day& in);

void BITCASK_EXPORT                     string_to_boost_ptime(std::string_view in, boost::posix_time::ptime& out);
boost::posix_time::ptime BITCASK_EXPORT string_to_boost_ptime(std::string_view in);

void BITCASK_EXPORT boost_ptime_to_string(const boost::posix_time::ptime& in, std::string& out, const char date_time_separator = 'T');
std::string BITCASK_EXPORT boost_ptime_to_string(const boost::posix_time::ptime& in, const char date_time_separator = 'T');

void BITCASK_EXPORT                   string_to_boost_date(std::string_view in, boost::gregorian::date& out);
boost::gregorian::date BITCASK_EXPORT string_to_boost_date(std::string_view in);

void BITCASK_EXPORT        boost_date_to_string(const boost::gregorian::date& in, std::string& out);
std::string BITCASK_EXPORT boost_date_to_string(const boost::gregorian::date& in);

void BITCASK_EXPORT                             string_to_boost_time_duration(std::string_view in, boost::posix_time::time_duration& out);
boost::posix_time::time_duration BITCASK_EXPORT string_to_boost_time_duration(std::string_view in);

void BITCASK_EXPORT        boost_time_duration_to_string(const boost::posix_time::time_duration& in, std::string& out);
std::string BITCASK_EXPORT boost_time_duration_to_string(const boost::posix_time::time_duration& in);

} // namespace bitcask
