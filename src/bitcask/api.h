//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

// clang-format off

#if defined _WIN32 || defined __CYGWIN__
  #define BITCASK_API_IMPORT __declspec(dllimport)
  #define BITCASK_API_EXPORT __declspec(dllexport)
  #define BITCASK_API_LOCAL
#else
  #if __GNUC__ >= 4 // TODO: clang?
    #define BITCASK_API_IMPORT __attribute__ ((visibility ("default")))
    #define BITCASK_API_EXPORT __attribute__ ((visibility ("default")))
    #define BITCASK_API_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define BITCASK_API_IMPORT
    #define BITCASK_API_EXPORT
    #define BITCASK_API_LOCAL
  #endif
#endif

#ifdef BITCASK_SHARED // compiled as a shared library
  #ifdef BITCASK_SHARED_EXPORTS // defined if we are building the shared library
    #define BITCASK_EXPORT BITCASK_API_EXPORT
  #else
    #define BITCASK_EXPORT BITCASK_API_IMPORT
  #endif // BITCASK_SHARED_EXPORTS
  #define BITCASK_LOCAL BITCASK_API_LOCAL
#else // compiled as a static library
  #define BITCASK_EXPORT
  #define BITCASK_LOCAL
#endif // BITCASK_SHARED
