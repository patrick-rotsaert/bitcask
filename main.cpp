#include "bitcask.h"
#include "test_operation.h"
#include "make_random_operations.h"
#include "counter_timer.h"
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

using map_type = std::map<key_type, value_type>;

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
		fmt::print(stderr, "Writing {}\n", test_operations_csv_file);
		make_random_operations(map, count, [&](test_operation op, std::string_view key, std::string_view value) {
			fmt::print(out, "{},{},{}\n", static_cast<int>(op), key, value);
		});
	}
	{
		auto out = std::ofstream{ test_map_file };
		fmt::print(stderr, "Writing {}\n", test_map_file);
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
		fmt::print(stderr, "OK. Keys in map: {}\n", map1.size());
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
		//fmt::print(stderr, "{}\n", ++linenr);
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
				trace("Get existing {}={}\n", key, value);
				(void)value;

				ct_get_exist.start();
				const auto res = bc_->get(key);
				ct_get_exist.stop();

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
			} while (false);
			break;
		case test_operation::get_nexist:
			do
			{
				trace("Get non-existing {}\n", key);

				ct_get_nexist.start();
				const auto res = bc_->get(key);
				ct_get_nexist.stop();

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
		case test_operation::ins:
			do
			{
				trace("Ins {}={}\n", key, value);

				ct_ins.start();
				const auto res = bc_->put(key, value);
				ct_ins.stop();

				trace(" -> {}\n", res);
				assert(res);
				(void)res;
			} while (false);
			break;
		case test_operation::upd:
			do
			{
				trace("Upd {}={}\n", key, value);

				ct_upd.start();
				const auto res = bc_->put(key, value);
				ct_upd.stop();

				trace(" -> {}\n", res);
				assert(!res);
				(void)res;
			} while (false);
			break;
		case test_operation::del_exist:
			do
			{
				trace("Del existing {}\n", key);

				ct_del_exist.start();
				const auto res = bc_->del(key);
				ct_del_exist.stop();

				trace(" -> {}\n", res);
				assert(res);
				(void)res;
			} while (false);
			break;
		case test_operation::del_nexist:
			do
			{
				trace("Del non-existing {}\n", key);

				ct_del_nexist.start();
				const auto res = bc_->del(key);
				ct_del_nexist.stop();

				trace(" -> {}\n", res);
				assert(!res);
				(void)res;
			} while (false);
			break;
		}
	}
};

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
	fmt::print(stderr, "Run started\n");
	run_random_operations(bc, map1, 1000u);
	fmt::print(stderr, "Run finished\n");
	const auto map2 = load_map(bc);
	verify_maps_are_equal(map1, map2);
}

void run_test_02()
{
	auto bc = bitcask{ bitcask_dir };
	bc.clear(); // !!!
	fmt::print(stderr, "Run started\n");
	run_test_operations_from_file(bc);
	fmt::print(stderr, "Run finished\n");
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
			fmt::print(stderr, "Run started\n");
			run_test_operations_from_file(bc);
			fmt::print(stderr, "Run finished\n");
		}
		map1 = load_map(bc);
	}
	{
		auto bc = bitcask{ bitcask_dir };
		//bc.max_file_size(10u * 1024u * 1024u);
		fmt::print(stderr, "Merge started\n");
		bc.merge();
		fmt::print(stderr, "Merge finished\n");
		map2 = load_map(bc);
	}
	verify_maps_are_equal(map1, map2);
	{
		fmt::print(stderr, "Load started\n");
		auto bc = bitcask{ bitcask_dir };
		map2    = load_map(bc);
		fmt::print(stderr, "Load finished\n");
	}
	verify_maps_are_equal(map1, map2);
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

int main()
{
	try
	{
		//load_map_from_file();
		//write_random_test_operations_to_file(1e6);
		//run_merge();
		//run_test_01();
		//run_test_02();
		run_test_03();
		//run_merge();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}
}
