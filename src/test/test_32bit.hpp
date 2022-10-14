/*
 * test_32bit.h
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#pragma once
#ifndef TEST_32BIT_HPP_
#define TEST_32BIT_HPP_

#include <cstdint>

#include <concurrentfw/aba_wrapper.hpp>

class Test32Bit
{
public:
	Test32Bit(int32_t init = 0)
	: test(init)
	{}

	void set(int32_t const value);
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

void Test32Bit::set(int32_t const value)
{
	bool (*inlined_modify_func)(const int32_t&, int32_t&, const int32_t) =	// implicit conversion
		[](const int32_t& /* ptr_cached */, int32_t& value_modify, const int32_t value_init) -> bool
	{
		value_modify = value_init;
		return true;
	};

	test.modify(inlined_modify_func, value);
}

#endif /* TEST_32BIT_HPP_ */
