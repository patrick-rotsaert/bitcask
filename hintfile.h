#pragma once

#include "file.h"
#include "basic_types.h"
#include "keydir.h"

#include <memory>

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

	void build_keydir(keydir& kd, file_id_type file_id);

	struct hint final
	{
		version_type     version;
		value_sz_type    value_sz;
		value_pos_type   value_pos;
		std::string_view key;
	};

	void put(hint&& rec);
};
