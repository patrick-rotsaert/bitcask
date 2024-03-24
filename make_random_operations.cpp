//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "make_random_operations.h"
#include "counter_timer.hpp"

#include <random>

namespace bitcask {
namespace demo {

void make_random_operations(std::map<key_type, value_type>&                                         map,
                            std::size_t                                                             count,
                            std::function<void(test_operation, std::string_view, std::string_view)> handler)
{
	constexpr auto empty = std::string_view{ "" };

	auto rd = std::random_device{};
	auto re = std::default_random_engine{ rd() };
	//auto re = std::mt19937{ rd() };

	auto&& pick_random_map_pair = [&]() {
		auto dist = std::uniform_int_distribution<std::size_t>(0, map.size() - 1);

		auto it = map.begin();
		//std::advance(it, dist(re)); // << very slow
		std::next(it, dist(re)); // << much faster

		return *it;
	};

	auto&& generate_random_string = [&](std::size_t length) -> std::string {
		constexpr auto chars  = std::string_view{ "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" };
		auto           dist   = std::uniform_int_distribution<std::size_t>(0, chars.length() - 1);
		auto           result = std::string{};
		result.resize(length);
		for (std::size_t i = 0; i < length; ++i)
		{
			result[i] = chars[dist(re)];
		}
		return result;
	};

	auto&& generate_random_key = [&]() {
		//auto dist = std::uniform_int_distribution<std::size_t>(2, 100);
		auto dist = std::uniform_int_distribution<std::size_t>(50, 200);
		return generate_random_string(dist(re));
	};

	auto&& generate_random_value = [&]() {
		auto dist = std::uniform_int_distribution<std::size_t>(0, 400);
		return generate_random_string(dist(re));
	};

	auto&& generate_random_key_non_existing = [&]() {
		for (;;)
		{
			const auto key = generate_random_key();
			if (map.contains(key))
			{
				continue;
			}
			else
			{
				return key;
			}
		}
	};

	//auto distribution = std::uniform_int_distribution<int>(static_cast<int>(action::first), static_cast<int>(action::last));
	auto distribution = std::discrete_distribution<>{
		2,  // get_exist
		.5, // get_nexist
		4,  // ins
		1,  // upd
		.5, // del_exist
		.1  // del_nexist
	};

	auto ct = counter_timer{};
	for (auto n = count; n; --n)
	{
		const auto _  = ct.raii_start();
		const auto op = static_cast<test_operation>(distribution(re));
		switch (op)
		{
		case test_operation::get_exist:
			if (!map.empty())
			{
				const auto pair = pick_random_map_pair();
				handler(op, pair.first, pair.second);
			}
			break;
		case test_operation::get_nexist:
			handler(op, generate_random_key_non_existing(), empty);
			break;
		case test_operation::ins:
			do
			{
				const auto key   = generate_random_key_non_existing();
				const auto value = generate_random_value();
				map[key]         = value;
				handler(op, key, value);
			} while (false);
			break;
		case test_operation::upd:
			if (!map.empty())
			{
				const auto key   = pick_random_map_pair().first;
				const auto value = generate_random_value();
				map[key]         = value;
				handler(op, key, value);
			}
			break;
		case test_operation::del_exist:
			if (!map.empty())
			{
				const auto key = pick_random_map_pair().first;
				map.erase(key);
				handler(op, key, empty);
			}
			break;
		case test_operation::del_nexist:
			handler(op, generate_random_key_non_existing(), empty);
			break;
		}
	}
	ct.report("loop");
}

} // namespace demo
} // namespace bitcask
