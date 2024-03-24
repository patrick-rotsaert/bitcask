#pragma once

#include "locktypes.hpp"
#include <filesystem>
#include <memory>
#include <sys/stat.h>

class file final
{
	class impl;
	std::unique_ptr<impl> pimpl_;

public:
	explicit file(int fd, const std::filesystem::path& path);
	~file() noexcept; // closes the descriptor

	file(file&&)            = delete;
	file& operator=(file&&) = delete;

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

	// These methods lock the mutex and release it on return.
	std::size_t read(void* buf, std::size_t count, read_mode mode) const;
	void        write(const void* buf, std::size_t count) const;
	off64_t     seek(off64_t offset, int whence) const;
	off64_t     seek(off64_t offset) const;
	off64_t     position() const;
	off64_t     size() const;

	// Lock this instance.
	// Use this lock if you need to perform several dependent operations. For example,
	// to perform a seek and a write, first get a lock, then pass that lock to locked_seek and locked_write.
	// For as long as the returned lock exists, do not call the non-locked methods obove. This would deadlock.
	lock_type lock() const;

	// Locked methods.
	// Useful only if compiled with BITCASK_THREAD_SAFE
	std::size_t locked_read(const lock_type&, void* buf, std::size_t count, read_mode mode) const;
	void        locked_write(const lock_type&, const void* buf, std::size_t count) const;
	off64_t     locked_seek(const lock_type&, off64_t offset, int whence) const;
	off64_t     locked_seek(const lock_type&, off64_t offset) const;
	off64_t     locked_position(const lock_type&) const;
	off64_t     locked_size(const lock_type&) const;
};
