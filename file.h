#pragma once

#include "hton.h"

#include <filesystem>
#include <memory>
#include <sys/stat.h>

class file final
{
	int                   fd_;
	off64_t               position_;
	std::filesystem::path path_;

public:
	explicit file(int fd, const std::filesystem::path& path);
	~file() noexcept; // closes the descriptor

	file(file&&)            = default;
	file& operator=(file&&) = default;

	file(const file&)            = delete;
	file& operator=(const file&) = delete;

	static std::unique_ptr<file> open(const std::filesystem::path& path, int flags, mode_t mode);

	void reopen(int flags, mode_t mode);

	const std::filesystem::path& path() const;

	enum class read_mode
	{
		any,
		zero_or_count,
		count
	};

	std::size_t read(void* buf, std::size_t count, read_mode mode);
	void        write(const void* buf, std::size_t count);
	off64_t     seek(off64_t offset, int whence);
	off64_t     seek(off64_t offset);
	off64_t     position() const;
	off64_t     size();
};
