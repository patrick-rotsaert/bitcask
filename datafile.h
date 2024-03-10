#pragma once

#include "file.h"
#include "basic_types.h"
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

	file_id_type id() const;

	file& get_file() const;

	std::filesystem::path path() const;
	std::filesystem::path hint_path() const;

	void build_keydir(keydir& kd);

	value_type   get(const keydir::info& info);
	keydir::info put(const std::string_view& key, const std::string_view& value, version_type version);
	void         del(const std::string_view& key, version_type version);

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

	void traverse(std::function<void(const record&)> callback);
};
