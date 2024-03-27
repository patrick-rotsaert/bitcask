//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "bitcask/version.h"

#include <cassert>

namespace bitcask {

int version::number()
{
	return BITCASK_VERSION_NUMBER;
}

int version::major()
{
	return BITCASK_VERSION_MAJOR;
}

int version::minor()
{
	return BITCASK_VERSION_MINOR;
}

int version::patch()
{
	return BITCASK_VERSION_PATCH;
}

void version::check()
{
	assert(BITCASK_VERSION_NUMBER == number());
}

} // namespace bitcask
