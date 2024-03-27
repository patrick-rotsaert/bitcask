//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "bitcask/keydir.h"
#include "bitcask/locktypes.hpp"

#include <string>
#include <unordered_map>
#include <stdexcept>

namespace bitcask {

class keydir::impl
{
	struct string_hash
	{
		using is_transparent = void;

		std::size_t operator()(const std::string& v) const
		{
			return std::hash<std::string>{}(v);
		}

		std::size_t operator()(const std::string_view& v) const
		{
			return std::hash<std::string_view>{}(v);
		}

		std::size_t operator()(const char* v) const
		{
			return std::hash<std::string_view>{}(v);
		}
	};

	std::unordered_map<key_type, keydir_info, string_hash, std::equal_to<>> map_;
	version_type                                                            version_;
	mutable shared_locker                                                   locker_;

public:
	impl() noexcept
	    : map_{}
	    , version_{}
	    , locker_{}
	{
	}

	version_type next_version()
	{
		const auto lock = this->locker_.write_lock();
		(void)(lock);

		return ++this->version_;
	}

	std::optional<keydir::info> get(const std::string_view& key) const
	{
		const auto lock = this->locker_.read_lock();
		(void)(lock);

		const auto it = this->map_.find(key);
		if (it == this->map_.end())
		{
			return std::nullopt;
		}
		else
		{
			return it->second;
		}
	}

	std::optional<std::pair<keydir::info*, write_lock_type>> get_mutable(const std::string_view& key)
	{
		auto lock = this->locker_.write_lock();
		(void)(lock);

		const auto it = this->map_.find(key);
		if (it == this->map_.end())
		{
			return std::nullopt;
		}
		else
		{
			return std::make_pair(&it->second, std::move(lock));
		}
	}

	bool empty() const
	{
		const auto lock = this->locker_.read_lock();
		(void)(lock);

		return this->map_.empty();
	}

	bool put(const std::string_view& key, keydir::info&& info)
	{
		const auto lock = this->locker_.write_lock();
		(void)(lock);

		if (info.version > this->version_)
		{
			this->version_ = info.version;
		}

		return this->map_.insert_or_assign(std::string{ key }, std::move(info)).second;
	}

	bool del(const std::string_view& key)
	{
		const auto lock = this->locker_.write_lock();
		(void)(lock);

		const auto it = this->map_.find(key);
		if (it == this->map_.end())
		{
			return false;
		}
		else
		{
			this->map_.erase(it);
			return true;
		}
	}

	bool traverse(std::function<bool(const std::string_view& key, const info& info)> callback)
	{
		const auto lock = this->locker_.read_lock();
		(void)(lock);

		for (const auto& pair : this->map_)
		{
			if (!callback(pair.first, pair.second))
			{
				return false;
			}
		}
		return true;
	}
};

keydir::keydir() noexcept
    : pimpl_{ std::make_unique<impl>() }
{
}

keydir::~keydir() noexcept
{
}

version_type keydir::next_version()
{
	return this->pimpl_->next_version();
}

std::optional<keydir::info> keydir::get(const std::string_view& key) const
{
	return this->pimpl_->get(key);
}

std::optional<std::pair<keydir::info*, write_lock_type>> keydir::get_mutable(const std::string_view& key)
{
	return this->pimpl_->get_mutable(key);
}

bool keydir::empty() const
{
	return this->pimpl_->empty();
}

bool keydir::put(const std::string_view& key, info&& info)
{
	return this->pimpl_->put(key, std::move(info));
}

bool keydir::del(const std::string_view& key)
{
	return this->pimpl_->del(key);
}

bool keydir::traverse(std::function<bool(const std::string_view&, const info&)> callback)
{
	return this->pimpl_->traverse(callback);
}

} // namespace bitcask
