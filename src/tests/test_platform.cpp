/*
 * test_platform.cpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <concurrentfw/version.hpp>

TEST_CASE("check minimum number of available threads", "[threads]")
{
	uint32_t hw_threads = std::thread::hardware_concurrency();
	INFO("available hardware threads: " << hw_threads);
	REQUIRE(hw_threads >= 2);  // github runner limit
}
