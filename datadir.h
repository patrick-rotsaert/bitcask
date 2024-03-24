#pragma once

#include "keydir.h"
#include "basic_types.h"

#include <filesystem>
#include <memory>

class datadir final
{
	class impl;
	std::unique_ptr<impl> pimpl_;

public:
	explicit datadir(const std::filesystem::path& directory);
	~datadir() noexcept;

	datadir(datadir&&)            = default;
	datadir& operator=(datadir&&) = default;

	datadir(const datadir&)            = delete;
	datadir& operator=(const datadir&) = delete;

	off64_t max_file_size() const;
	void    max_file_size(off64_t size);

	void build_keydir(keydir& kd);

	value_type   get(const keydir::info& info);
	keydir::info put(const std::string_view& key, const std::string_view& value, version_type version);
	void         del(const std::string_view& key, version_type version);

	// maintenance
	void merge(keydir& kd);

	// destruction
	// make sure no datadir instance exists with this directory!
	static void clear(const std::filesystem::path& directory);
};
