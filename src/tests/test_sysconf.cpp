/*
* test_sysconf.cpp
*
* (C) 2023 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
*
* This file is distributed under the MIT license, see file LICENSE.
*/

#include <catch2/catch_test_macros.hpp>

#include <concurrentfw/sysconf.hpp>

TEST_CASE("check of sysconf access", "[sysconf]")
{
    CHECK_THROWS_AS(ConcurrentFW::sysconf<size_t>(-1), std::system_error);
    CHECK_NOTHROW(ConcurrentFW::cache_line());
    CHECK_NOTHROW(ConcurrentFW::page_size());
    CHECK(ConcurrentFW::cache_line() == 64);
    CHECK(ConcurrentFW::page_size() == 4096);
}
