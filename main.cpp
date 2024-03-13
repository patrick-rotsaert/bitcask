#include "bitcask.h"
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <stdexcept>
#include <chrono>
#include <iostream>
#include <cassert>
#include <map>
#include <random>
#include <type_traits>
#include <chrono>

using map_type = std::map<key_type, value_type>;

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

#if defined(DEBUG)
#define trace(...)                                                                                                                         \
	do                                                                                                                                     \
	{                                                                                                                                      \
		fmt::print(stderr, __VA_ARGS__);                                                                                                   \
	} while (false)
#else
#define trace(...)                                                                                                                         \
	do                                                                                                                                     \
	{                                                                                                                                      \
	} while (false)
#endif

void run_test_01(bitcask& bc)
{
	bc.max_file_size(10u * 1024u * 1024u); // 10M

	auto map = map_type{};

	bc.traverse([&](const auto& key, const auto& value) {
		map[std::string{ key }] = value;
		return true;
	});

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

	auto&& generate_random_value = [&]() {
		//auto dist = std::uniform_int_distribution<std::size_t>(0, 1000);
		auto dist = std::uniform_int_distribution<std::size_t>(0, 10);
		return generate_random_string(dist(re));
	};

	auto ct_get = counter_timer{};
	auto ct_put = counter_timer{};
	auto ct_del = counter_timer{};

	for (auto n = 1000000; n; --n)
	{
		enum class action
		{
			first     = 0,
			get_exist = first,
			get_nexist,
			ins,
			upd,
			del_exist,
			del_nexist,
			last = del_nexist
		};

		auto dist = std::uniform_int_distribution<int>(static_cast<int>(action::first), static_cast<int>(action::last));
		switch (static_cast<action>(dist(re)))
		{
		case action::get_exist:
			if (!map.empty())
			{
				const auto pair = pick_random_map_pair();

				const auto& key   = pair.first;
				const auto& value = pair.second;

				trace("Get existing {}={}\n", key, value);
				(void)value;

				ct_get.start();
				const auto res = bc.get(key);
				ct_get.stop();

				if (res.has_value())
				{
					trace(" -> {}\n", res.value());
				}
				else
				{
					trace(" -> null\n");
				}
				assert(res.has_value());
				assert(res.value() == value);
			}
			break;
		case action::get_nexist:
			do
			{
				const auto key = generate_random_key_non_existing();

				trace("Get non-existing {}\n", key);

				ct_get.start();
				const auto res = bc.get(key);
				ct_get.stop();

				if (res.has_value())
				{
					trace(" -> {}\n", res.value());
				}
				else
				{
					trace(" -> null\n");
				}
				assert(!res.has_value());
			} while (false);
			break;
		case action::ins:
			do
			{
				const auto key   = generate_random_key_non_existing();
				const auto value = generate_random_value();

				trace("Ins {}={}\n", key, value);
				map[key] = value;

				ct_put.start();
				const auto res = bc.put(key, value);
				ct_put.stop();

				trace(" -> {}\n", res);
				assert(res);
				(void)res;
			} while (false);
			break;
		case action::upd:
			if (!map.empty())
			{
				const auto key   = pick_random_map_pair().first;
				const auto value = generate_random_value();

				trace("Upd {}={} (was {})\n", key, value, map[key]);
				map[key] = value;

				ct_put.start();
				const auto res = bc.put(key, value);
				ct_put.stop();

				trace(" -> {}\n", res);
				assert(!res);
				(void)res;
			}
			break;
		case action::del_exist:
			if (!map.empty())
			{
				const auto key = pick_random_map_pair().first;

				trace("Del existing {}\n", key);
				map.erase(key);

				ct_del.start();
				const auto res = bc.del(key);
				ct_del.stop();

				trace(" -> {}\n", res);
				assert(res);
				(void)res;
			}
			break;
		case action::del_nexist:
			do
			{
				const auto key = generate_random_key_non_existing();

				trace("Del non-existing {}\n", key);

				ct_del.start();
				const auto res = bc.del(key);
				ct_del.stop();

				trace(" -> {}\n", res);
				assert(!res);
				(void)res;
			} while (false);
			break;
		}
	}

	ct_get.report("get");
	ct_put.report("put");
	ct_del.report("del");
}

void run_test_02(bitcask& bc)
{
	bc.put("tomato", "fruit");
	bc.put("tomato", "vegetable");

	auto res = bc.get("tomato");
	if (res.has_value())
	{
		trace(" -> {}\n", res.value());
	}
	else
	{
		trace(" -> null\n");
	}
}

void run_merge(bitcask& bc)
{
	auto ct = counter_timer{};

	ct.start();
	bc.merge();
	ct.stop();

	ct.report("merge");
}

int main()
{
	try
	{
		const auto start = std::chrono::steady_clock::now();

		auto bc = bitcask{ "/tmp/bitcask" };

		{
			const auto now = std::chrono::steady_clock::now();
			trace("Bitcask construction took {}s\n", std::chrono::duration<double>{ now - start }.count());
			(void)start;
			(void)now;
		}

		//run_test_01(bc);
		//run_test_02(bc);
		run_merge(bc);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}
}
