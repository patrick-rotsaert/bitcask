//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <boost/describe.hpp>

namespace bitcask {

BOOST_DEFINE_ENUM_CLASS(log_level, trace, debug, info, warn, err, critical, off)

}
