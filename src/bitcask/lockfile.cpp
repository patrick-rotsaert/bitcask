//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "bitcask/lockfile.h"

#if defined(_WIN32)
#error not implemented
#else
#include "bitcask/lockfile_impl_posix.hpp"
#endif

namespace bitcask {

lockfile::lockfile(const std::filesystem::path& path)
    : pimpl_{ std::make_unique<impl>(path) }
{
}

lockfile::~lockfile() noexcept
{
}

} // namespace bitcask
