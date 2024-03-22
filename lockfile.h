#pragma once

#include <filesystem>
#include <memory>

class lockfile
{
	class impl;
	std::unique_ptr<impl> pimpl_;

public:
	lockfile(const std::filesystem::path& path);
	~lockfile() noexcept;

	lockfile(lockfile&&)            = default;
	lockfile& operator=(lockfile&&) = default;

	lockfile(const lockfile&)            = delete;
	lockfile& operator=(const lockfile&) = delete;
};
