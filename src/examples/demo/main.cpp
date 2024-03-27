//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "bitcask/bitcask.h"
#include "bitcask/config.h"
#include "bitcask/logging.h"
#include "test_operation.h"
#include "make_random_operations.h"
#include "counter_timer.hpp"
#include "syncqueue.hpp"
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cassert>
#include <map>
#include <type_traits>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <random>
#include <future>

#ifdef BITCASK_USE_SPDLOG
#include <spdlog/spdlog.h>
#endif

namespace bitcask {
namespace demo {

using map_type = std::map<key_type, value_type>;

const auto bitcask_dir              = std::filesystem::path{ "/tmp/bitcask" };
const auto test_operations_csv_file = bitcask_dir / "test_operations.csv";
const auto test_map_file            = bitcask_dir / "test_map.csv";

map_type load_map(bitcask& bc)
{
	auto map = map_type{};

	bc.traverse([&](const auto& key, const auto& value) {
		map[std::string{ key }] = value;
		return true;
	});

	return map;
}

void write_random_test_operations_to_file(std::size_t count = 1000)
{
	// Assuming the key and value never contain a ','
	auto map = map_type{};
	{
		auto out = std::ofstream{ test_operations_csv_file };
		BITCASK_LOG(debug, "Writing {}", test_operations_csv_file);
		make_random_operations(map, count, [&](test_operation op, std::string_view key, std::string_view value) {
			fmt::print(out, "{},{},{}\n", static_cast<int>(op), key, value);
		});
	}
	{
		auto out = std::ofstream{ test_map_file };
		BITCASK_LOG(debug, "Writing {}", test_map_file);
		for (const auto& [key, value] : map)
		{
			fmt::print(out, "{},{}\n", key, value);
		}
	}
}

std::vector<std::string_view> split_string(std::string_view in, char sep)
{
	auto out   = std::vector<std::string_view>{};
	auto begin = std::string_view::size_type{};
	for (;;)
	{
		const auto end = in.find_first_of(sep, begin);
		out.push_back(in.substr(begin, end - begin));
		if (end == std::string_view::npos)
		{
			break;
		}
		else
		{
			begin = end + 1;
		}
	}
	return out;
}

map_type load_map_from_file()
{
	// Assuming the key and value never contain a ','
	auto map  = map_type{};
	auto in   = std::ifstream{ test_map_file };
	auto line = std::string{};
	while (std::getline(in, line))
	{
		const auto fields = split_string(line, ',');
		if (fields.size() == 2u)
		{
			map[std::string{ fields[0] }] = fields[1];
		}
	}
	return map;
}

void verify_maps_are_equal(const map_type& map1, const map_type& map2)
{
	if (map1 == map2)
	{
		BITCASK_LOG(debug, "OK. Keys in map: {}", map1.size());
	}
	else
	{
		throw std::runtime_error{ fmt::format("FAIL. Maps differ. Keys in 1st map: {}, in 2nd {}\n", map1.size(), map2.size()) };
	}
}

void traverse_test_operations_from_file(std::function<void(test_operation op, std::string_view key, std::string_view value)> handler)
{
	auto in   = std::ifstream{ test_operations_csv_file };
	auto line = std::string{};
	//auto linenr = std::size_t{};
	while (std::getline(in, line))
	{
		const auto fields = split_string(line, ',');
		if (fields.size() == 3u)
		{
			handler(static_cast<test_operation>(std::stoi(std::string{ fields[0] })), fields[1], fields[2]);
		}
	}
}

struct test_operation_executor
{
	bitcask*      bc_{};
	counter_timer ct_get_exist{};
	counter_timer ct_get_nexist{};
	counter_timer ct_ins{};
	counter_timer ct_upd{};
	counter_timer ct_del_exist{};
	counter_timer ct_del_nexist{};
	counter_timer ct_iterate{};

	test_operation_executor(bitcask& bc)
	    : bc_{ &bc }
	{
		bc_->max_file_size(
		    //
		    10u * 1024u * 1024u // 10M
		                        //1u * 1024u * 1024u // 1M
		);
	}

	~test_operation_executor()
	{
		ct_get_exist.report("get exist");
		ct_get_nexist.report("get nexist");
		ct_ins.report("ins");
		ct_upd.report("upd");
		ct_del_exist.report("del exist");
		ct_del_nexist.report("del nexist");
		ct_iterate.report("iterate");
	}

	test_operation_executor(test_operation_executor&&)            = delete;
	test_operation_executor& operator=(test_operation_executor&&) = delete;

	test_operation_executor(const test_operation_executor&)            = delete;
	test_operation_executor& operator=(const test_operation_executor&) = delete;

	void operator()(test_operation op, std::string_view key, std::string_view value)
	{
		const auto _ = ct_iterate.raii_start();
		switch (op)
		{
		case test_operation::get_exist:
			do
			{
				BITCASK_LOG(trace, "Get existing {}={}", key, value);
				(void)value;

				ct_get_exist.start();
				const auto res = bc_->get(key);
				ct_get_exist.stop();

				if (res.has_value())
				{
					BITCASK_LOG(trace, " -> {}", res.value());
				}
				else
				{
					BITCASK_LOG(trace, " -> null");
				}
				assert(res.has_value());
				assert(res.value() == value);
			} while (false);
			break;
		case test_operation::get_nexist:
			do
			{
				BITCASK_LOG(trace, "Get non-existing {}", key);

				ct_get_nexist.start();
				const auto res = bc_->get(key);
				ct_get_nexist.stop();

				if (res.has_value())
				{
					BITCASK_LOG(trace, " -> {}", res.value());
				}
				else
				{
					BITCASK_LOG(trace, " -> null");
				}
				assert(!res.has_value());
			} while (false);
			break;
		case test_operation::ins:
			do
			{
				BITCASK_LOG(trace, "Ins {}={}", key, value);

				ct_ins.start();
				const auto res = bc_->put(key, value);
				ct_ins.stop();

				BITCASK_LOG(trace, " -> {}", res);
				assert(res);
				(void)res;
			} while (false);
			break;
		case test_operation::upd:
			do
			{
				BITCASK_LOG(trace, "Upd {}={}", key, value);

				ct_upd.start();
				const auto res = bc_->put(key, value);
				ct_upd.stop();

				BITCASK_LOG(trace, " -> {}", res);
				assert(!res);
				(void)res;
			} while (false);
			break;
		case test_operation::del_exist:
			do
			{
				BITCASK_LOG(trace, "Del existing {}", key);

				ct_del_exist.start();
				const auto res = bc_->del(key);
				ct_del_exist.stop();

				BITCASK_LOG(trace, " -> {}", res);
				assert(res);
				(void)res;
			} while (false);
			break;
		case test_operation::del_nexist:
			do
			{
				BITCASK_LOG(trace, "Del non-existing {}", key);

				ct_del_nexist.start();
				const auto res = bc_->del(key);
				ct_del_nexist.stop();

				BITCASK_LOG(trace, " -> {}", res);
				assert(!res);
				(void)res;
			} while (false);
			break;
		}
	}
};

#ifdef BITCASK_THREAD_SAFE
class worker
{
	bitcask&                              bc_;
	synchronized_queue<std::string_view>& queue_;
	std::thread                           thread_;
	std::size_t                           get_count_;

public:
	explicit worker(bitcask& bc, synchronized_queue<std::string_view>& queue)
	    : bc_{ bc }
	    , queue_{ queue }
	    , thread_{ std::bind(&worker::run, this) }
	    , get_count_{}
	{
	}

	~worker()
	{
		this->join();
	}

	void join()
	{
		if (this->thread_.joinable())
		{
			this->thread_.join();
		}
	}

	auto get_count() const
	{
		return this->get_count_;
	}

private:
	void run()
	{
		for (;;)
		{
			auto key = std::string_view{};
			if (this->queue_.pop(key))
			{
				auto res = this->bc_.get(key);
				assert(res.has_value());
				(void)res;

				++this->get_count_;
			}
			else
			{
				return;
			}
		}
	}
};

#endif

void run_test_operations_from_file(bitcask& bc)
{
	auto executor = test_operation_executor{ bc };
	traverse_test_operations_from_file(std::ref(executor));
}

void run_random_operations(bitcask& bc, map_type& map, std::size_t count = 1000u)
{
	auto executor = test_operation_executor{ bc };
	make_random_operations(map, count, std::ref(executor));
}

void run_test_01()
{
	auto bc   = bitcask{ bitcask_dir };
	auto map1 = load_map(bc);
	BITCASK_LOG(debug, "Run started");
	run_random_operations(bc, map1, 1000u);
	BITCASK_LOG(debug, "Run finished");
	const auto map2 = load_map(bc);
	verify_maps_are_equal(map1, map2);
}

void run_test_02()
{
	bitcask::clear(bitcask_dir); // !!!
	auto bc = bitcask{ bitcask_dir };
	BITCASK_LOG(debug, "Run started");
	run_test_operations_from_file(bc);
	BITCASK_LOG(debug, "Run finished");
	const auto map1 = load_map(bc);
	const auto map2 = load_map_from_file();
	verify_maps_are_equal(map1, map2);
}

void run_test_03()
{
	auto map1 = map_type{};
	auto map2 = map_type{};
	{
		auto bc = bitcask{ bitcask_dir };
		if (bc.empty())
		{
			if (!std::filesystem::exists(test_operations_csv_file) || !std::filesystem::exists(test_map_file))
			{
				write_random_test_operations_to_file(1e6);
			}
			BITCASK_LOG(debug, "Run started");
			run_test_operations_from_file(bc);
			BITCASK_LOG(debug, "Run finished");
		}
		map1 = load_map(bc);
	}
	{
		auto bc = bitcask{ bitcask_dir };
		//bc.max_file_size(10u * 1024u * 1024u);
		BITCASK_LOG(debug, "Merge started");
		bc.merge();
		BITCASK_LOG(debug, "Merge finished");
		map2 = load_map(bc);
	}
	verify_maps_are_equal(map1, map2);
	{
		BITCASK_LOG(debug, "Load started");
		auto bc = bitcask{ bitcask_dir };
		map2    = load_map(bc);
		BITCASK_LOG(debug, "Load finished");
	}
	verify_maps_are_equal(map1, map2);
}

void run_concurrency_test_01()
{
#ifdef BITCASK_THREAD_SAFE
	BITCASK_LOG(debug, "Load started");
	auto bc  = bitcask{ bitcask_dir };
	auto map = load_map(bc);
	BITCASK_LOG(debug, "Load finished");

	auto keys = std::vector<std::string_view>{};
	std::transform(map.begin(), map.end(), std::back_inserter(keys), [](const auto& pair) { return std::string_view{ pair.first }; });

	using clock_type = std::chrono::high_resolution_clock;

	//const auto num_threads = std::thread::hardware_concurrency();
	const auto num_threads = 4u;

	auto threads = std::vector<std::thread>{};

	auto durations = std::vector<clock_type::duration>{};
	durations.resize(num_threads);

	while (threads.size() < num_threads)
	{
		auto& duration = durations[threads.size()];

		threads.emplace_back([&]() {
			auto my_keys = keys;

			auto rd = std::random_device{};
			auto re = std::default_random_engine{ rd() };

			std::shuffle(my_keys.begin(), my_keys.end(), re);

			const auto start = clock_type::now();

			std::for_each(my_keys.begin(), my_keys.end(), [&](const auto& key) {
				auto res = bc.get(key);
				assert(res.has_value());
				(void)res;
			});

			const auto finish = clock_type::now();
			duration          = finish - start;
		});
	}

	BITCASK_LOG(debug, "Joining {} threads", num_threads);

	std::for_each(threads.begin(), threads.end(), [](auto& thread) { thread.join(); });

	const auto total_duration = std::accumulate(durations.begin(), durations.end(), clock_type::duration{});

	BITCASK_LOG(info,
	            "Total duration for {} threads: {}\n"
	            "Keys gotten per thread: {}\n"
	            "Avg per get: {}",
	            num_threads,
	            total_duration,
	            keys.size(),
	            total_duration / (num_threads * keys.size()));
#else
	fmt::print(stderr, "Concurrency test not possible\n");
#endif
}

void run_concurrency_test_02()
{
#ifdef BITCASK_THREAD_SAFE
	BITCASK_LOG(debug, "Load started");
	auto bc  = bitcask{ bitcask_dir };
	auto map = load_map(bc);
	BITCASK_LOG(debug, "Load finished");

	if (map.empty())
	{
		BITCASK_LOG(info, "The bitcask is empty, nothing to do...");
		return;
	}

	auto queue = synchronized_queue<std::string_view>{};
	std::for_each(map.begin(), map.end(), [&](const auto& pair) { queue.push(std::string_view{ pair.first }); });

	assert(queue.size() == map.size());

	using clock_type = std::chrono::high_resolution_clock;

	const auto num_workers = std::thread::hardware_concurrency();
	//const auto num_workers = 1u;

	auto workers = std::vector<std::unique_ptr<worker>>{};

	const auto start = clock_type::now();

	while (workers.size() < num_workers)
	{
		workers.push_back(std::make_unique<worker>(bc, queue));
	}

	std::for_each(workers.begin(), workers.end(), [](auto& worker) { worker->join(); });

	assert(queue.empty());

	const auto finish = clock_type::now();

	std::for_each(workers.begin(), workers.end(), [](auto& worker) {
		BITCASK_LOG(debug, "Worker get count: {}", worker->get_count());
		(void)worker;
	});

	workers.clear();

	const auto duration = finish - start;

	BITCASK_LOG(info,
	            "Total duration for {} workers: {}\n"
	            "Keys gotten per thread: {}\n"
	            "Avg per key: {}",
	            num_workers,
	            duration,
	            map.size(),
	            duration / map.size());

#else
	fmt::print(stderr, "Concurrency test not possible\n");
#endif
}

void run_merge()
{
	auto bc = bitcask{ bitcask_dir };

	bc.max_file_size(10u * 1024u * 1024u);

	auto ct = counter_timer{};

	ct.start();
	bc.merge();
	ct.stop();

	ct.report("merge");
}

} // namespace demo
} // namespace bitcask

int main()
{
#ifdef BITCASK_USE_SPDLOG
	spdlog::set_level(spdlog::level::trace);
	spdlog::set_pattern("%L [%Y-%m-%d %H:%M:%S.%f Δt=%iμs](%t) %^%v%$ [%s:%#]");
#endif

	bclog(debug, "application started");

	using namespace bitcask::demo;

	try
	{
		//load_map_from_file();
		///write_random_test_operations_to_file(1e6);
		//run_merge();
		//run_test_01();
		///run_test_02();
		//run_test_03();
		//run_merge();
		//run_concurrency_test_01();
		run_concurrency_test_02();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}
}
