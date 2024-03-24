#pragma once

#include <fmt/format.h>
#include <fmt/chrono.h>

#include <chrono>

struct counter_timer
{
	std::size_t              count{};
	std::chrono::nanoseconds dur{};

	using clock_type = std::chrono::high_resolution_clock;
	clock_type::time_point tp{};

	struct stopper
	{
		counter_timer& ct_;

		explicit stopper(counter_timer& ct)
		    : ct_{ ct }
		{
		}

		~stopper()
		{
			const auto tp = clock_type::now();
			this->ct_.dur += tp - this->ct_.tp;

			++this->ct_.count;
		}
	};

	void start()
	{
		this->tp = clock_type::now();
	}

	stopper raii_start()
	{
		this->tp = clock_type::now();
		return stopper{ *this };
	}

	void stop()
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
