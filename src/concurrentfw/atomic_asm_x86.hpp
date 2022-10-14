/*
 * concurrentfw/atomic_asm_x86.h
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#pragma once
#ifndef CONCURRENTFW_ATOMIC_ASM_X86_HPP_
#define CONCURRENTFW_ATOMIC_ASM_X86_HPP_

#if defined __x86_64__ || defined __i686__

#if !defined(__GCC_ASM_FLAG_OUTPUTS__)
#error "No GCC inline assembler with flag outputs"
#endif

#include <cstdint>

#define ATOMIC_DWCAS_NEEDED

// The atomic loops are unlikely repeated, therefore skip the alignment overhead before the loop
#define GNU_OPTIMIZE_ATOMIC_LOOPS_ALIGNMENT "align-loops=1"

// see: https://www.boost.org/doc/libs/1_55_0/boost/atomic/detail/gcc-x86.hpp

namespace ConcurrentFW
{

#if defined(__x86_64__)

inline static void atomic_dw_load(volatile uint64_t atomic[2], uint64_t target[2]) noexcept
{
	asm volatile  // clang-format off
	(
		"movq %%rbx, %%rax\n\t"			// rbx:rcx (desired) -> rax:rdx, rbx:rcx is unmodified
		"movq %%rcx, %%rdx\n\t"			// rax:rdx (expected) is early clobber and overwritten before cmpxchg16b
		"lock cmpxchg16b (%[ptr])"
		:		"=&a"	(target[0])		// rax: output only, early clobber
		,		"=&d"	(target[1])		// rdx: output only, early clobber
		: [ptr]	"r"		(atomic)
		:		"cc"					// flags modified. cmpxchg16b is a full memory barrier, therefore "memory" is not needed here
	);	// clang-format on
}

inline static void atomic_dw_store(volatile uint64_t atomic[2], const uint64_t desired[2]) noexcept
{
	asm volatile  // clang-format off
	(
		"movq 0(%[ptr]), %%rax\n\t"			// *ptr -> rax:rdc (non-atomic)
		"movq 8(%[ptr]), %%rdx\n\t"			// rax:rdx (expected) is therefore actually stored value
		"1: lock cmpxchg16b (%[ptr])\n\t"	// try write
		"jne 1b"							// 'b'ackward jump
		:
		:		"b" (desired[0])
		,		"c" (desired[1])
		, [ptr]	"r" (atomic)
		:		"cc", "rax", "rdx"			// flags & rax:rdx modified. cmpxchg16b is a full memory barrier, therefore "memory" is not needed here
	);	// clang-format on
}

inline static bool atomic_dw_cas(volatile uint64_t atomic[2], uint64_t expected[2], const uint64_t desired[2]) noexcept
{
	bool exchanged;
	asm volatile  // clang-format off
	(
		"lock cmpxchg16b (%[ptr])"
		: 		"=@ccz"	(exchanged)		// flag z is output
		,		"+a"	(expected[0])	// rax: input/output
		,		"+d"	(expected[1])	// rdx: input/output
		:		"b"		(desired[0])	// rbx: input only
		,		"c"		(desired[1])	// rcx: input only
		, [ptr]	"r"		(atomic)
		:								// cmpxchg16b is a full memory barrier, therefore "memory" is not needed here
	);	// clang-format on
	return exchanged;
}

#endif

inline static void atomic_dw_load(volatile uint32_t atomic[2], uint32_t destination[2]) noexcept
{
#if defined(__x86_64__)
	asm volatile  // clang-format off
	(
		"movq (%[ptr]), %%rax\n\t"			// guaranteed to be atomic
		"shldq $32,%%rax,%%rdx"				// result is stored in eax:edx
		:		"=a"	(destination[0])	// eax: output only
		,		"=d"	(destination[1])	// edx: output only
		: [ptr]	"r"		(atomic)
		:		"cc"						// flags modified.
	);	// clang-format on
#elif defined(__i686__)
	asm volatile  // clang-format off
	(
		"movl %%ebx, %%eax\n\t"		// eax:edx is early clobber and overwritten before cmpxchg16b
		"movl %%ecx, %%edx\n\t"
		"lock cmpxchg8b (%[ptr])"
		:		"=&a"	(reinterpret_cast<uint32_t*>(destination)[0])	// eax: output only, early clobber
		,		"=&d"	(reinterpret_cast<uint32_t*>(destination)[1])	// edx: output only, early clobber
		: [ptr]	"r"		(atomic)
		:		"cc"	// flags modified. cmpxchg8b is a full memory barrier, therefore "memory" is not needed here
	);	// clang-format on
#endif
}

inline static void atomic_dw_store(volatile uint32_t atomic[2], const uint32_t desired[2]) noexcept
{
#if defined(__x86_64__)
	[[maybe_unused]] uint32_t unused1;	// will be discarded
	[[maybe_unused]] uint32_t unused2;	// will be discarded
	asm volatile  // clang-format off
	(
		"shlq $32,%%rbx\n\t"			// data is stored in ebx:ecx
		"shldq $32,%%rbx,%%rcx\n\t"
		"movq %%rcx, (%[ptr])"			// guaranteed to be atomic
		:		"=b" (unused1)			// rbx is modified, discard it
		,		"=c" (unused2)			// rcx is modified, discard it
		:		"b" (desired[0])
		,		"c" (desired[1])
		, [ptr]	"r" (atomic)
		:		"cc"					// flags modified.
	);								// clang-format on
#elif defined(__i686__)
	//	https://stackoverflow.com/questions/48046591/how-do-i-atomically-move-a-64bit-value-in-x86-asm
	asm volatile  // clang-format off
	(
		"movl 0(%[ptr]), %%eax\n\t"			// *ptr -> eax:edc (non-atomic)
		"movl 4(%[ptr]), %%edx\n\t"
		"1: lock cmpxchg8b (%[ptr])\n\t"	// try write
		"jne 1b"							// 'b'ackward jump
		:
		:		"b" (reinterpret_cast<const uint32_t*>(desired)[0])
		,		"c" (reinterpret_cast<const uint32_t*>(desired)[1])
		, [ptr]	"r" (atomic)
		:		"cc", "eax", "edx"			// flags & rax:rdx modified. cmpxchg16b is a full memory barrier, therefore "memory" is not needed here
	);	// clang-format on
#endif
}

inline static bool atomic_dw_cas(volatile uint32_t atomic[2], uint32_t expected[2], const uint32_t desired[2])
{
	bool exchanged;
	asm volatile  // clang-format off
	(
		"lock cmpxchg8b (%[ptr])"
		:		"=@ccz"	(exchanged)		// flag z is output
		,		"+a"	(expected[0])	// eax: input/output
		,		"+d"	(expected[1])	// edx: input/output
		:		"b"		(desired[0])	// ebx: input only
		,		"c"		(desired[1])	// ecx: input only
		, [ptr]	"r"		(atomic)
		:								// cmpxchg8b is a full memory barrier, therefore "memory" is not needed here
	);	// clang-format on
	return exchanged;
}

}  // namespace ConcurrentFW

#endif	// __x86_64__ || __i686__

#endif	// CONCURRENTFW_ATOMIC_ASM_X86_HPP_
