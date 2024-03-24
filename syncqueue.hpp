//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace bitcask {

template<typename T>
class synchronized_queue
{
	std::queue<T>           queue_{};
	mutable std::mutex      mutex_{};
	std::condition_variable popCondition_{};

	using lock_type = std::unique_lock<std::mutex>;

public:
	void push(T&& data)
	{
		auto lock = lock_type{ this->mutex_ };
		this->queue_.push(std::move(data));
		lock.unlock();
		this->popCondition_.notify_one();
	}

	bool pop(T& data)
	{
		auto lock = lock_type{ this->mutex_ };
		if (this->queue_.empty())
		{
			return false;
		}
		data = std::move(this->queue_.front());
		this->queue_.pop();
		return true;
	}

	bool empty() const
	{
		auto lock = lock_type{ this->mutex_ };
		return this->queue_.empty();
	}

	std::size_t size() const
	{
		auto lock = lock_type{ this->mutex_ };
		return this->queue_.size();
	}
};

} // namespace bitcask
