#pragma once

#include "basic_types.h"

#include <filesystem>
#include <memory>
#include <optional>

class bitcask final
{
	class impl;
	std::unique_ptr<impl> pimpl_;

public:
	explicit bitcask(const std::filesystem::path& directory);
	~bitcask() noexcept;

	bitcask(bitcask&&)            = default;
	bitcask& operator=(bitcask&&) = default;

	bitcask(const bitcask&)            = delete;
	bitcask& operator=(const bitcask&) = delete;

	off64_t max_file_size() const;
	void    max_file_size(off64_t size);

	std::optional<value_type> get(const std::string_view& key);
	void                      put(const std::string_view& key, const std::string_view& value); // FIXME: return update count
	void                      del(const std::string_view& key);                                // FIXME: return update count

	// maintenance
	void merge();
};
