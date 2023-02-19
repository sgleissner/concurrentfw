/*
 * test_x86_asm_helper.cpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#ifdef X86_64_FAST_DW_LOAD
#undef X86_64_FAST_DW_LOAD
#endif
#ifdef X86_64_FAST_DW_STORE
#undef X86_64_FAST_DW_STORE
#endif
#include <concurrentfw/atomic_asm_x86.hpp>

#if defined(__x86_64__)

void slow_atomic_dw_load(volatile uint64_t atomic[2], uint64_t target[2])
{
	ConcurrentFW::atomic_dw_load(atomic, target);
}

void slow_atomic_dw_store(volatile uint64_t atomic[2], const uint64_t desired[2])
{
	ConcurrentFW::atomic_dw_store(atomic, desired);
}

#endif

#if defined(__x86_64__) || defined(__i686__)

void slow_atomic_dw_load(volatile uint32_t atomic[2], uint32_t target[2])
{
	ConcurrentFW::atomic_dw_load(atomic, target);
}

void slow_atomic_dw_store(volatile uint32_t atomic[2], const uint32_t desired[2])
{
	ConcurrentFW::atomic_dw_store(atomic, desired);
}

#endif
