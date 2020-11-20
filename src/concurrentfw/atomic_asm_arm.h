/*
 * concurrentfw/atomic_asm_arm.h
 *
 * (C) 2017-2020 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#ifndef CONCURRENTFW_ATOMIC_ASM_ARM_H_
#define CONCURRENTFW_ATOMIC_ASM_ARM_H_

#if defined __arm__ || defined __aarch64__

#if !defined(__GCC_ASM_FLAG_OUTPUTS__)
#error "No GCC inline assembler with flag outputs"
#endif

#include <cstdint>

#define ATOMIC_LLSC_NEEDED

#if __ARM_ARCH < 7	// minimum ARMv7, shall be ARMv6K in the future
#error "ARMv7 or higher needed"
#endif

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
	asm volatile
	(
		"clrex"
	);
}

#endif

inline static void atomic_exclusive_abort(volatile uint32_t& /*atomic*/)
{
	asm volatile
	(
		"clrex"
	);
}

#if defined(__aarch64__)

// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0802a/LDXR.html

// ldaxr documentation
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100076_0100_00_en/pge1427897401219.html

inline static void atomic_exclusive_load_aquire(volatile uint64_t& atomic, uint64_t& target)
{
	asm volatile
	(
		"ldaxr %0, [%1]"
		: "=r" (target)
		: "r" (&atomic)
		:
	);
}

inline static void atomic_exclusive_load_aquire(volatile uint32_t& atomic, uint32_t& target)
{
	asm volatile
	(
		"ldaxr %w0, [%1]"
		: "=r" (target)
		: "r" (&atomic)
		:
	);
}

// stlxr documentation
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100076_0100_00_en/pge1427897423561.html

inline static bool atomic_exclusive_store_release(volatile uint64_t& atomic, const uint64_t& store)
{
	bool failed;
	asm volatile
	(
		"stlxr %w0, %1, [%2]"
		: "=r" (failed)
		: "r" (store)
		, "r" (&atomic)
		:
	);
	return !failed;
}

inline static bool atomic_exclusive_store_release(volatile uint32_t& atomic, const uint32_t& store)
{
	bool failed;
	asm volatile
	(
		"stlxr %w0, %w1, [%2]"
		: "=r" (failed)
		: "r" (store)
		, "r" (&atomic)
		:
	);
	return !failed;
}

#elif defined (__arm__)

#if __ARM_ARCH >= 8	// ARMv8 and higher

inline static void atomic_exclusive_load_aquire(volatile uint32_t& atomic, uint32_t& target)
{
	asm volatile
	(
		"ldaex %0, [%1]"
		: "=r" (target)
		: "r" (&atomic)
		:
	);
}

inline static bool atomic_exclusive_store_release(volatile uint32_t& atomic, const uint32_t& store)
{
	bool failed;
	asm volatile
	(
		"stlex %0, %1, [%2]"
		: "=r" (failed)
		: "r" (store)
		, "r" (&atomic)
		:
	);
	return !failed;
}

#elif __ARM_ARCH >= 7	// ARMv7

inline static void atomic_exclusive_load_aquire(volatile uint32_t& atomic, uint32_t& target)
{
	asm volatile
	(
		"ldrex %0, [%1]\n\t"
		"dmb"					// full memory fence
		: "=r" (target)
		: "r" (&atomic)
		:
	);
}

inline static bool atomic_exclusive_store_release(volatile uint32_t& atomic, const uint32_t& store)
{
	bool failed;
	asm volatile
	(
		"dmb\n\t"				// full memory fence
		"strex %0, %1, [%2]"
		: "=r" (failed)
		: "r" (store)
		, "r" (&atomic)
		:
	);
	return !failed;
}

#else	// ARMv6K

#error "memory barrier code still missing for ARMv6K"

#endif	// ARMv6K

#endif	// __arm__

}		// namespace ConcurrentFW

#endif	// __arm__ || defined __aarch64__

#endif	// CONCURRENTFW_ATOMIC_ASM_ARM_H_
