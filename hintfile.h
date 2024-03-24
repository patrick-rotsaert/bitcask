//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "file.h"
#include "basictypes.h"
#include "keydir.h"

#include <memory>

namespace bitcask {

class hintfile final
{
	class impl;
	std::unique_ptr<impl> pimpl_;

public:
	explicit hintfile(std::unique_ptr<file>&& f);
	~hintfile() noexcept;

	hintfile(hintfile&&)            = default;
	hintfile& operator=(hintfile&&) = default;

	hintfile(const hintfile&)            = delete;
	hintfile& operator=(const hintfile&) = delete;

	std::filesystem::path path() const;

	void build_keydir(keydir& kd, file_id_type file_id) const;

	struct hint final
	{
		version_type     version;
		value_sz_type    value_sz;
		value_pos_type   value_pos;
		std::string_view key;
	};

	void put(hint&& rec) const;
};

} // namespace bitcask
