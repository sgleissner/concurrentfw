/*
 * test_concurrent_ptr.cpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

#include <concurrentfw/concurrent_ptr.hpp>

TEST_CASE("check Concurrent_Ptr", "[concurrent_ptr]")
{
    ConcurrentFW::Concurrent_Ptr<uint16_t> test_ptr;  // pointer test
    auto counter = test_ptr.get_counter();
    CHECK(test_ptr.alignment == (1 + ConcurrentFW::ABA_IS_PLATFORM_DWCAS) * sizeof(void*));
    uint16_t x1 = 42;
    uint16_t x2 = 4711;
    uint16_t x3 = 0x0815;
    test_ptr.set(&x1);
    CHECK(*test_ptr.get() == 42);
    test_ptr.set(&x2);
    CHECK(*test_ptr.get() == 4711);
    test_ptr.set(&x3);
    CHECK(*test_ptr.get() == 0x0815);
    CHECK(test_ptr.get_counter() - counter == (3 * ConcurrentFW::ABA_IS_PLATFORM_DWCAS));
}
