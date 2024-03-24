#pragma once

#include "config.h"

#ifdef BITCASK_THREAD_SAFE
#include <mutex>
#include <shared_mutex>

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

#else

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
#endif
