#include "bitcask.h"
#include <fmt/format.h>
#include <stdexcept>
#include <chrono>
#include <iostream>
#include <cassert>
#include <map>
#include <random>
#include <type_traits>

using map_type = std::map<key_type, value_type>;

void run_test_01(bitcask& bc)
{
	bc.max_file_size(10u * 1024u * 1024u); // 10M

	enum class action
	{
		first = 0,
		get   = first,
		ins,
		upd,
		del,
		last = del
	};

	auto map = map_type{};

	auto rd = std::random_device{};
	auto re = std::default_random_engine{ rd() };

	auto&& pick_random_map_pair = [&]() {
		auto dist = std::uniform_int_distribution<std::size_t>(0, map.size() - 1);

		auto it = map.begin();
		std::advance(it, dist(re));

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
		auto dist = std::uniform_int_distribution<std::size_t>(1, 10);
		return generate_random_string(dist(re));
	};

	auto&& generate_random_value = [&]() {
		//auto dist = std::uniform_int_distribution<std::size_t>(0, 1000);
		auto dist = std::uniform_int_distribution<std::size_t>(0, 10);
		return generate_random_string(dist(re));
	};

	for (auto n = 100000; n; --n)
	{
		auto dist = std::uniform_int_distribution<int>(static_cast<int>(action::first), static_cast<int>(action::last));
		switch (static_cast<action>(dist(re)))
		{
		case action::get:
			if (!map.empty())
			{
				const auto pair = pick_random_map_pair();

				const auto& key   = pair.first;
				const auto& value = pair.second;

				fmt::print(stderr, "Get {}={}\n", key, value);
				auto res = bc.get(key);
				if (res.has_value())
				{
					fmt::print(stderr, " -> {}\n", res.value());
				}
				else
				{
					fmt::print(stderr, " -> null\n");
				}
				assert(res.has_value());
				assert(res.value() == value);
			}
			break;
		case action::ins:
			for (;;)
			{
				const auto key = generate_random_key();
				if (map.contains(key))
				{
					continue;
				}

				const auto value = generate_random_value();

				fmt::print(stderr, "Ins {}={}\n", key, value);
				map[key] = value;
				bc.put(key, value);

				break;
			}
			break;
		case action::upd:
			if (!map.empty())
			{
				const auto key   = pick_random_map_pair().first;
				const auto value = generate_random_value();

				fmt::print(stderr, "Upd {}={} (was {})\n", key, value, map[key]);
				map[key] = value;
				bc.put(key, value);
			}
			break;
		case action::del:
			if (!map.empty())
			{
				const auto key = pick_random_map_pair().first;

				fmt::print(stderr, "Del {}\n", key);
				map.erase(key);
				bc.del(key);
			}
			break;
		}
	}
}

void run_test_02(bitcask& bc)
{
	bc.put("tomato", "fruit");
	bc.put("tomato", "vegetable");

	auto res = bc.get("tomato");
	if (res.has_value())
	{
		fmt::print(stderr, " -> {}\n", res.value());
	}
	else
	{
		fmt::print(stderr, " -> null\n");
	}
}

int main()
{
	try
	{
		const auto start = std::chrono::steady_clock::now();

		auto bc = bitcask{ "/tmp/bitcask" };

		{
			const auto now = std::chrono::steady_clock::now();
			fmt::print(stderr, "Bitcask construction took {}s\n", std::chrono::duration<double>{ now - start }.count());
		}

		//run_test_01(bc);
		run_test_02(bc);
		//bc.merge();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}
}
