//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "bitcask/basictypes.h"
#include "bitcask/locktypes.hpp"

#include <memory>
#include <string_view>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <functional>

namespace bitcask {

struct keydir_info final
{
	file_id_type   file_id;
	value_sz_type  value_sz;
	value_pos_type value_pos;
	version_type   version;
};

class keydir final
{
	class impl;
	std::unique_ptr<impl> pimpl_;

public:
	using info = keydir_info;

	keydir() noexcept;
	~keydir() noexcept;

	version_type next_version();

	std::optional<info>                              get(const std::string_view& key) const;
	std::optional<std::pair<info*, write_lock_type>> get_mutable(const std::string_view& key);

	bool empty() const;

	/// Returns true if the key was inserted, false if the key existed.
	bool put(const std::string_view& key, info&& info);

	/// Returns true if the key was deleted, false if the key did not exist.
	bool del(const std::string_view& key);

	bool traverse(std::function<bool(const std::string_view& key, const info& info)> callback);
};

} // namespace bitcask
