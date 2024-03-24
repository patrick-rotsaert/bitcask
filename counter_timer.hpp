//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <fmt/format.h>
#include <fmt/chrono.h>

#include <chrono>

namespace bitcask {
namespace demo {

struct counter_timer
{
	std::size_t              count{};
	std::chrono::nanoseconds dur{};

	using clock_type = std::chrono::high_resolution_clock;
	clock_type::time_point tp{};

	struct stopper
	{
		counter_timer& ct_;

		explicit stopper(counter_timer& ct) noexcept
		    : ct_{ ct }
		{
		}

		~stopper() noexcept
		{
			const auto tp = clock_type::now();
			this->ct_.dur += tp - this->ct_.tp;

			++this->ct_.count;
		}
	};

	void start() noexcept
	{
		this->tp = clock_type::now();
	}

	stopper raii_start() noexcept
	{
		this->tp = clock_type::now();
		return stopper{ *this };
	}

	void stop() noexcept
	{
		const auto tp = clock_type::now();
		this->dur += tp - this->tp;

		++this->count;
	}

	void report(std::string_view name)
	{
		if (this->count)
		{
			fmt::print(stdout, "{}: count={} total={} avg={}\n", name, this->count, this->dur, this->dur / this->count);
		}
		else
		{
			fmt::print(stdout, "{}: count={}\n", name, this->count);
		}
	}
};

} // namespace demo
} // namespace bitcask
