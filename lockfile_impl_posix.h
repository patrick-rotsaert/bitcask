#include "lockfile.h"
#include <system_error>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

class lockfile::impl
{
	std::filesystem::path path_;
	int                   fd_;

public:
	explicit impl(const std::filesystem::path& path)

	    : path_{ path }
	    , fd_{ open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600) }
	{
		if (this->fd_ == -1)
		{
			throw std::system_error{ std::error_code{ errno, std::system_category() }, path.string() + ": open" };
		}

		auto fl     = flock{};
		fl.l_type   = F_WRLCK;
		fl.l_whence = SEEK_SET;
		fl.l_start  = 0;
		fl.l_len    = 0;

		if (fcntl(this->fd_, F_SETLK, &fl) == -1)
		{
			throw std::system_error{ std::error_code{ errno, std::system_category() }, path.string() + ": fcntl(F_SETLK)" };
		}
	}

	~impl() noexcept
	{
		auto ec = std::error_code{};
		std::filesystem::remove(this->path_, ec);
		close(this->fd_);
	}
};
