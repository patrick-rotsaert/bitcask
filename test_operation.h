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
