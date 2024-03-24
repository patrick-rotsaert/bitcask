#pragma once

#include "test_operation.h"
#include "basictypes.h"
#include <map>
#include <functional>

void make_random_operations(std::map<key_type, value_type>&                                           map,
                            std::size_t                                                               count,
                            std::function<void (test_operation, std::string_view, std::string_view)> handler);
