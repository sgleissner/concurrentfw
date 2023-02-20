/*
 * concurrentfw/atomic_asm_arm.h
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#pragma once
#ifndef CONCURRENTFW_ATOMIC_ASM_ARM_HPP_
#define CONCURRENTFW_ATOMIC_ASM_ARM_HPP_

#if defined __arm__ || defined __aarch64__

#if !defined(__GCC_ASM_FLAG_OUTPUTS__)
#error "No GCC inline assembler with flag outputs"
#endif

#include <cstdint>

#define ATOMIC_LLSC_NEEDED

#if __ARM_ARCH < 7	// minimum ARMv7, shall be ARMv6K in the future
#error "ARMv7 or higher needed"
#endif

// ARM LL/SC sequences shall be localized within one cache line for best performance
#define GNU_OPTIMIZE_ATOMIC_LOOPS_ALIGNMENT "align-loops=64"

// "For reasons of performance, keep the number of instructions between corresponding LDREX and STREX instructions to a minimum."
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0204f/Cihbghef.html

// ARM contraints: https://gcc.gnu.org/onlinedocs/gcc/Machine-Constraints.html#Machine-Constraints

namespace ConcurrentFW
{

// clrex documentation
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100076_0100_00_en/pge1427897657895.html

// clrex is available since ARM1176JZF-S (Raspberry Pi 1)
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0301h/Chdgjcag.html

#if defined(__aarch64__)

inline static void atomic_exclusive_abort(volatile uint64_t& /*atomic*/)
{
	asm volatile  // clang-format off
	(
		"clrex"
	);  // clang-format on
}

#endif

inline static void atomic_exclusive_abort(volatile uint32_t& /*atomic*/)
{
	asm volatile  // clang-format off
	(
		"clrex"
	);  // clang-format on
}

#if defined(__aarch64__)

// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0802a/LDXR.html

// ldaxr documentation
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100076_0100_00_en/pge1427897401219.html

inline static void atomic_exclusive_load_aquire(volatile uint64_t& atomic, uint64_t& target)
{
	asm volatile  // clang-format off
	(
		"ldaxr %0, [%1]"
		: "=r" (target)
		: "r" (&atomic)
		: "memory"			// "memory" acts as compiler r/w barrier
	);	// clang-format on
}

inline static void atomic_exclusive_load_aquire(volatile uint32_t& atomic, uint32_t& target)
{
	asm volatile  // clang-format off
	(
		"ldaxr %w0, [%1]"
		: "=r" (target)
		: "r" (&atomic)
		: "memory"			// "memory" acts as compiler r/w barrier
	);	// clang-format on
}

// stlxr documentation
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100076_0100_00_en/pge1427897423561.html

inline static bool atomic_exclusive_store_release(volatile uint64_t& atomic, const uint64_t& store)
{
	bool failed;
	asm volatile  // clang-format off
	(
		"stlxr %w0, %1, [%2]"
		: "=&r" (failed)	// early clobber + read/write because of unpredictable case: identical transfer and status registers
		: "r" (store),
		  "r" (&atomic)
		: "memory"			// "memory" acts as compiler r/w barrier
	);	// clang-format on
	return !failed;
}

inline static bool atomic_exclusive_store_release(volatile uint32_t& atomic, const uint32_t& store)
{
	bool failed;
	asm volatile  // clang-format off
	(
		"stlxr %w0, %w1, [%2]"
		: "=&r" (failed)	// early clobber + read/write because of unpredictable case: identical transfer and status registers
		: "r" (store),
		  "r" (&atomic)
		: "memory"			// "memory" acts as compiler r/w barrier
	);	// clang-format on
	return !failed;
}

#elif defined(__arm__)

#if __ARM_ARCH >= 8	 // ARMv8 and higher

inline static void atomic_exclusive_load_aquire(volatile uint32_t& atomic, uint32_t& target)
{
	asm volatile  // clang-format off
	(
		"ldaex %0, [%1]"
		: "=r" (target)
		: "r" (&atomic)
		: "memory"			// "memory" acts as compiler r/w barrier
	);	// clang-format on
}

inline static bool atomic_exclusive_store_release(volatile uint32_t& atomic, const uint32_t& store)
{
	bool failed;
	asm volatile  // clang-format off
	(
		"stlex %0, %1, [%2]"
		: "=r" (failed)
		: "r" (store),
		  "r" (&atomic)
		: "memory"			// "memory" acts as compiler r/w barrier
	);	// clang-format on
	return !failed;
}

#elif __ARM_ARCH >= 7  // ARMv7

inline static void atomic_exclusive_load_aquire(volatile uint32_t& atomic, uint32_t& target)
{
	asm volatile  // clang-format off
	(
		"ldrex %0, [%1]\n\t"
		"dmb"					// full memory fence
		: "=r" (target)
		: "r" (&atomic)
		: "memory"			// "memory" acts as compiler r/w barrier
	);	// clang-format on
}

inline static bool atomic_exclusive_store_release(volatile uint32_t& atomic, const uint32_t& store)
{
	bool failed;
	asm volatile  // clang-format off
	(
		"dmb\n\t"				// full memory fence
		"strex %0, %1, [%2]"
		: "=r" (failed)
		: "r" (store),
		  "r" (&atomic)
		: "memory"			// "memory" acts as compiler r/w barrier
	);	// clang-format on
	return !failed;
}

#else  // ARMv6K

#error "memory barrier code still missing for ARMv6K"

#endif	// ARMv6K

#endif	// __arm__

}  // namespace ConcurrentFW

#endif	// __arm__ || defined __aarch64__

#endif	// CONCURRENTFW_ATOMIC_ASM_ARM_HPP_
