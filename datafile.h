#pragma once

#include "file.h"
#include "basictypes.h"
#include "keydir.h"
#include "hintfile.h"

#include <memory>
#include <regex>
#include <filesystem>
#include <optional>
#include <functional>

class datafile final
{
	class impl;
	std::unique_ptr<impl> pimpl_;

public:
	static std::regex name_regex;

	static std::string make_filename(file_id_type id);

	explicit datafile(std::unique_ptr<file>&& f);
	~datafile() noexcept;

	datafile(datafile&&)            = default;
	datafile& operator=(datafile&&) = default;

	datafile(const datafile&)            = delete;
	datafile& operator=(const datafile&) = delete;

	file_id_type          id() const;
	std::filesystem::path path() const;
	std::filesystem::path hint_path() const;

	static std::filesystem::path hint_path(const std::filesystem::path& path);

	bool size_greater_than(off64_t size) const;
	void reopen(int flags, mode_t mode) const;

	void build_keydir(keydir& kd) const;

	value_type   get(const keydir::info& info) const;
	keydir::info put(const std::string_view& key, const std::string_view& value, version_type version) const;
	void         del(const std::string_view& key, version_type version) const;

	struct record
	{
		struct value_info
		{
			value_pos_type   value_pos;
			std::string_view value;
			version_type     version;
		};

		std::string_view          key;
		std::optional<value_info> value;
	};

	void traverse(std::function<void(const record&)> callback) const;
};
