//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "test_operation.h"
#include "bitcask/basictypes.h"

#include <map>
#include <functional>

namespace bitcask {
namespace demo {

void make_random_operations(std::map<key_type, value_type>&                                         map,
                            std::size_t                                                             count,
                            std::function<void(test_operation, std::string_view, std::string_view)> handler);

}
} // namespace bitcask
