#pragma once

#include <string>
#include <cstdint>
#include <ctime>

#include <sys/types.h>

using crc_type = std::uint32_t;
//using version_type   = std::time_t;
using version_type   = std::uint64_t;
using ksz_type       = std::uint32_t;
using value_sz_type  = std::uint64_t;
using value_pos_type = off64_t;

using file_id_type = std::uint64_t;

using key_type   = std::string;
using value_type = std::string;
