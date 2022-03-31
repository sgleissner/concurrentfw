/*
 * test_aba.cpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#include <iostream>
#include <cstdint>

#include <concurrentfw/concurrent_ptr.h>
#include "test_32bit.h"


void test_aba()
{

	ConcurrentFW::Concurrent_Ptr<uint16_t> test;

	std::cout << "Alignment: " << ConcurrentFW::ABA_ATOMIC_ALIGNMENT<uint16_t> << std::endl;

	uint16_t x1 = 42;
	uint16_t x2 = 4711;
	uint16_t x3 = 0x0815;

	test.set(&x1);
	test.set(&x2);
	test.set(&x3);


	uint16_t* y = test.get();

	std::cout << *y << " " << test.get_counter() << std::endl;

	Test32Bit test32(1234567890);

	test32.set(-2000000000);

	std::cout << "32-Bit Test: "<< test32.get() << " " << test32.get_counter() << std::endl;

}

