/*
 * test_aba.cpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

#include <concurrentfw/aba_wrapper.hpp>

struct Test32Bit
{
    Test32Bit(int32_t init = 0)
    : test(init)
    {}

    void set [[ATTRIBUTE_ABA_LOOP_OPTIMIZE]] (int32_t const value)
    {
        test.modify(
            [&](const int32_t& /* ptr_cached */, int32_t& value_modify) -> bool
            {
                value_modify = value;
                return true;
            }
        );
#if 0
		bool (*inlined_modify_func)(const int32_t&, int32_t&, const int32_t) =	// implicit conversion
			[](const int32_t& /* ptr_cached */, int32_t& value_modify, const int32_t value_init) -> bool
		{
			value_modify = value_init;
			return true;
		};

		test.modify_funcptr(inlined_modify_func, value);
#endif
    }

    int32_t get()
    {
        return test.get();
    }

    uint32_t get_counter()
    {
        return test.get_counter();
    }

    ConcurrentFW::ABA_Wrapper<int32_t> test;
};

TEST_CASE("check ABA wrapper", "[aba_wrapper]")
{
    Test32Bit test_int32(1234567890);  // 32 bit test
    auto counter = test_int32.get_counter();
    CHECK(test_int32.test.alignment == ((1 + ConcurrentFW::ABA_IS_PLATFORM_DWCAS) * sizeof(int32_t)));
    test_int32.set(-2000000000);
    CHECK(test_int32.get() == -2000000000);
    test_int32.set(2000000000);
    CHECK(test_int32.get() == 2000000000);
    CHECK(test_int32.get_counter() - counter == (2 * ConcurrentFW::ABA_IS_PLATFORM_DWCAS));
}
