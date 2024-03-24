//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "config.h"

#ifdef BITCASK_THREAD_SAFE
#include <mutex>
#include <shared_mutex>

namespace bitcask {

using lock_type       = std::unique_lock<std::mutex>;
using write_lock_type = std::unique_lock<std::shared_mutex>;
using read_lock_type  = std::shared_lock<std::shared_mutex>;

class locker
{
	std::mutex mutex_{};

public:
	lock_type lock()
	{
		return lock_type{ this->mutex_ };
	}
};

class shared_locker
{
	std::shared_mutex mutex_{};

public:
	read_lock_type read_lock()
	{
		return read_lock_type{ this->mutex_ };
	}

	write_lock_type write_lock()
	{
		return write_lock_type{ this->mutex_ };
	}
};

} // namespace bitcask

#else

namespace bitcask {

struct lock_type
{
	void unlock()
	{
	}
};
using write_lock_type = lock_type;
using read_lock_type  = lock_type;

class locker
{
public:
	lock_type lock()
	{
		return lock_type{};
	}
};

class shared_locker
{
public:
	read_lock_type read_lock()
	{
		return read_lock_type{};
	}

	write_lock_type write_lock()
	{
		return write_lock_type{};
	}
};

} // namespace bitcask

#endif
