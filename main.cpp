#include "bitcask.h"
#include <fmt/format.h>
#include <stdexcept>
#include <chrono>
#include <iostream>
#include <cassert>

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

		bc.put("hello", "world!");

		bc.put("the key", "this is the value");

		{
			const auto value = bc.get("hello");
			assert(value.has_value());
			assert(value.value() == "world!");
		}

		bc.del("hello");

		{
			const auto value = bc.get("hello");
			assert(!value.has_value());
		}

		bc.merge();

		{
			const auto value = bc.get("the key");
			assert(value.has_value());
			assert(value.value() == "this is the value");
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}
}
