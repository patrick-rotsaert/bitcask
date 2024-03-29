//
// Copyright (C) 2023 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <fmt/core.h>

#include <filesystem>
#include <iomanip>
#include <sstream>

namespace fmt {

//template<typename T>
//struct formatter<std::optional<T>> : formatter<T>
//{
//	template<typename FormatContext>
//	auto format(const std::optional<T>& opt, FormatContext& ctx) const
//	{
//		if (opt)
//		{
//			fmt::formatter<T>::format(opt.value(), ctx);
//			return ctx.out();
//		}
//		return fmt::format_to(ctx.out(), "--");
//	}
//};

template<>
struct formatter<std::filesystem::path> : formatter<string_view>
{
	auto format(const std::filesystem::path& value, format_context& ctx) const
	{
		auto ss = std::ostringstream{};
		ss << std::quoted(value.string());
		return fmt::format_to(ctx.out(), "{}", ss.str());
	}
};

} // namespace fmt
