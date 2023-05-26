/*
 * test_x86_asm.cpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#include <catch2/catch_test_macros.hpp>

#ifndef X86_64_FAST_DW_LOAD
#define X86_64_FAST_DW_LOAD
#endif
#ifndef X86_64_FAST_DW_STORE
#define X86_64_FAST_DW_STORE
#endif

#include <concurrentfw/atomic_asm_x86.hpp>

#if defined(__x86_64__)

void slow_atomic_dw_load(uint64_t atomic[2], uint64_t target[2]);
void slow_atomic_dw_store(uint64_t atomic[2], const uint64_t desired[2]);

void fast_atomic_dw_load(uint64_t atomic[2], uint64_t target[2])
{
	ConcurrentFW::atomic_dw_load(atomic, target);
}

void fast_atomic_dw_store(uint64_t atomic[2], const uint64_t desired[2])
{
	ConcurrentFW::atomic_dw_store(atomic, desired);
}

#endif

#if defined(__x86_64__) || defined(__i686__)

void slow_atomic_dw_load(uint32_t atomic[2], uint32_t target[2]);
void slow_atomic_dw_store(uint32_t atomic[2], const uint32_t desired[2]);

void fast_atomic_dw_load(uint32_t atomic[2], uint32_t target[2])
{
	ConcurrentFW::atomic_dw_load(atomic, target);
}

void fast_atomic_dw_store(uint32_t atomic[2], const uint32_t desired[2])
{
	ConcurrentFW::atomic_dw_store(atomic, desired);
}

#endif


#if defined(__x86_64__)

TEST_CASE("check of x86 64 bit double word cas", "[x86]")
{
	alignas(16) uint64_t atomic[2] {0xDEADBEEF01234567, 0x1CEDCAFE89ABCDEF};

	uint64_t fast_result[2] {1, 2};
	fast_atomic_dw_load(atomic, fast_result);

	uint64_t slow_result[2] {3, 4};
	slow_atomic_dw_load(atomic, slow_result);

	CHECK(((fast_result[0] == slow_result[0]) && (fast_result[1] == slow_result[1])));

	uint64_t store_value[2] {0x0123456789ABCDEF, 0xFEDCBA9876543210};
	alignas(16) uint64_t fast_atomic[2] {5, 6};
	fast_atomic_dw_store(fast_atomic, store_value);

	alignas(16) uint64_t slow_atomic[2] {7, 8};
	slow_atomic_dw_store(slow_atomic, store_value);

	CHECK(((fast_atomic[0] == slow_atomic[0]) && (fast_atomic[1] == slow_atomic[1])));
}

#endif

#if defined(__x86_64__) || defined(__i686__)

TEST_CASE("check of x86 32 bit double word cas", "[x86]")
{
	alignas(8) uint32_t atomic[2] {0xDEADBEEF, 0x1CEDCAFE};

	uint32_t fast_result[2] {1, 2};
	fast_atomic_dw_load(atomic, fast_result);

	uint32_t slow_result[2] {3, 4};
	slow_atomic_dw_load(atomic, slow_result);

	CHECK(((fast_result[0] == slow_result[0]) && (fast_result[1] == slow_result[1])));

	uint32_t store_value[2] {0x01234567, 0xFEDCBA98};
	alignas(16) uint32_t fast_atomic[2] {5, 6};
	fast_atomic_dw_store(fast_atomic, store_value);

	alignas(16) uint32_t slow_atomic[2] {7, 8};
	slow_atomic_dw_store(slow_atomic, store_value);

	CHECK(((fast_atomic[0] == slow_atomic[0]) && (fast_atomic[1] == slow_atomic[1])));
}

#endif