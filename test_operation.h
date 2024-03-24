//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <string_view>

enum class test_operation
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
