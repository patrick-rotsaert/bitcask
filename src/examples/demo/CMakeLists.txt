#
# Copyright (C) 2024 Patrick Rotsaert
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#

set(TARGET bitcask_demo)
add_executable(${TARGET}
	main.cpp
	test_operation.h
	make_random_operations.cpp
	make_random_operations.h
	counter_timer.hpp
	syncqueue.hpp
)

target_compile_features(${TARGET} PRIVATE cxx_std_20)
target_link_libraries(${TARGET} PRIVATE bitcask::bitcask)
