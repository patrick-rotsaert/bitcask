#include "lockfile.h"

#if defined(_WIN32)
#error not implemented
#else
#include "lockfile_impl_posix.h"
#endif

lockfile::lockfile(const std::filesystem::path& path)
    : pimpl_{ std::make_unique<impl>(path) }
{
}

lockfile::~lockfile() noexcept
{
}
