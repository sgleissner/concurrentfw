/*
 * test_main.cpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#include <iostream>
#include <cstdint>

#include <concurrentfw/version.h>

void test_aba();
void test_benchmark_futex();

void print_version()
{
	std::cout << std::endl
			  << "Tests & benchmarks for version " << ConcurrentFW::Version::MAJOR << "."
			  << ConcurrentFW::Version::MINOR << "." << ConcurrentFW::Version::PATCH << std::endl;
}

void print_test_header(const char* const name)
{
	static uint32_t test_nr = 0;

	test_nr++;

	std::cout << "Test " << test_nr << ": " << name << std::endl << std::endl;
}

int main()
{
	print_version();

	print_test_header("Futex");
	test_benchmark_futex();

	print_test_header("ABA");
	test_aba();

	return 0;
}
