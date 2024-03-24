//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#if defined(_WIN32)
#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321
#define BYTE_ORDER LITTLE_ENDIAN
#else
#include <endian.h>
#endif

#include <type_traits>
#include <cstdint>

inline uint16_t swap(uint16_t x)
{
#if defined(__GNUC__) || defined(__clang__)
	return __builtin_bswap16(x);
#else
	return (x << 8) | (x >> 8);
#endif
}

inline uint32_t swap(uint32_t x)
{
#if defined(__GNUC__) || defined(__clang__)
	return __builtin_bswap32(x);
#else
	return (x >> 24) | ((x >> 8) & 0x0000FF00) | ((x << 8) & 0x00FF0000) | (x << 24);
#endif
}

inline uint64_t swap(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__)
	return __builtin_bswap64(x);
#else
	return ((x << 56) & 0xFF00000000000000ULL) | ((x << 40) & 0x00FF000000000000ULL) | ((x << 24) & 0x0000FF0000000000ULL) |
	       ((x << 8) & 0x000000FF00000000ULL) | ((x >> 8) & 0x00000000FF000000ULL) | ((x >> 24) & 0x0000000000FF0000ULL) |
	       ((x >> 40) & 0x000000000000FF00ULL) | ((x >> 56) & 0x00000000000000FFULL);
#endif
}

template<typename T>
inline std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, T> swap(T x)
{
	return static_cast<T>(swap(static_cast<std::make_unsigned_t<T>>(x)));
}

template<typename T>
inline std::enable_if_t<std::is_integral_v<T>, T> hton(T x)
{
#if BYTE_ORDER == BIG_ENDIAN
	return x;
#elif BYTE_ORDER == LITTLE_ENDIAN
	return swap(x);
#else
#error "Unknown byte order"
#endif
}

#define ntoh(x) hton(x)
