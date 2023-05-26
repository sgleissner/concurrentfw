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

#include <concurrentfw/helper.hpp>

#define ATOMIC_DWCAS_NEEDED

// The atomic loops are unlikely repeated, therefore skip the alignment overhead before the loop
#define GNU_OPTIMIZE_ATOMIC_LOOPS_ALIGNMENT "align-loops=1"

// see: https://www.boost.org/doc/libs/1_55_0/boost/atomic/detail/gcc-x86.hpp

// TODO: clarify memory order for SSE version
// https://rigtorp.se/isatomic/

namespace ConcurrentFW
{

#if defined(__x86_64__)

ALWAYS_INLINE static void atomic_dw_load(uint64_t atomic[2], uint64_t target[2]) noexcept
{
#ifdef X86_64_FAST_DW_LOAD
	[[maybe_unused]] __int128_t sse_temp;  // trick allows random scratch sse register
	compiler_barrier();
	__atomic_thread_fence(__ATOMIC_RELEASE);
	asm volatile  // clang-format off
	(
		"movdqa (%[ptr]), %[sse]\n\t"	// 128 bit aligned SSE read, atomic: see https://rigtorp.se/isatomic/
		"movq %[sse], %%rax\n\t"		// low(xmm0) -> rax
		"punpckhqdq %[sse], %[sse]\n\t"	// high(xmm0) -> low(xmm0)
		"movq %[sse], %%rdx"			// low(xmm0) -> rdx
		:		"=a"	(target[0]),	// rax: output only, no early clobber, as written after usage of [ptr]
				"=d"	(target[1]),	// rdx: output only, no early clobber, as written after usage of [ptr]
		  [sse] "=&x"	(sse_temp)		// [sse]: random scratch sse register as dummy early clobber result
		: [ptr]	"r"		(atomic)		// [ptr]: double word atomic address
		:		"memory"				// flags not modified, "memory" acts as compiler r/w barrier
	);								   // clang-format on
#else
	asm volatile  // clang-format off
	(
		"movq %%rbx, %%rax\n\t"			// rbx:rcx (desired) -> rax:rdx, rbx:rcx is unmodified
		"movq %%rcx, %%rdx\n\t"			// rax:rdx (expected) is early clobber and overwritten before cmpxchg16b
		"lock cmpxchg16b (%[ptr])"		// read double word, store in rax:rdx, eventually write same value
		:		"=&a"	(target[0]),	// rax: output only, early clobber, as written before usage of [ptr]
				"=&d"	(target[1])		// rdx: output only, early clobber, as written before usage of [ptr]
		: [ptr]	"r"		(atomic)		// [ptr]: double word atomic address
		:		"cc", "memory"			// flags modified. "memory" acts as compiler r/w barrier.
	);	// clang-format on
#endif
}

ALWAYS_INLINE static void atomic_dw_store(uint64_t atomic[2], const uint64_t desired[2]) noexcept
{
#ifdef X86_64_FAST_DW_STORE
	compiler_barrier();
	__atomic_thread_fence(__ATOMIC_RELEASE);
	[[maybe_unused]] __int128_t sse_temp;  // trick allows random scratch sse register
	asm volatile  // clang-format off
	(
		"movq %%rbx, %[sse]\n\t"
		"pinsrq $1, %%rcx, %[sse]\n\t"
		"movdqa %[sse], (%[ptr])\n\t"		// 128 bit aligned SSE read, atomic: see https://rigtorp.se/isatomic/
		: [sse] "=&x"	(sse_temp)			// [sse]: random scratch sse register as dummy early clobber result
		:		"b"		(desired[0]),
				"c"		(desired[1]),
		  [ptr]	"r"		(atomic)			// [ptr]: double word atomic address
		:		"memory"					// flags not modified, "memory" acts as compiler r/w barrier
	);								   // clang-format on
#else
	asm volatile  // clang-format off
	(
		"movq 0(%[ptr]), %%rax\n\t"				// *ptr -> rax:rdc (non-atomic)
		"movq 8(%[ptr]), %%rdx\n\t"				// rax:rdx (expected) is therefore actually stored value
		"1: lock cmpxchg16b (%[ptr])\n\t"		// try write
		"jne 1b"								// 'b'ackward jump
		:
		:		"b" (desired[0]),
				"c" (desired[1]),
		  [ptr]	"r" (atomic)					// [ptr]: double word atomic address
		:		"cc", "rax", "rdx", "memory"	// flags & rax:rdx modified, "memory" acts as compiler r/w barrier
	);	// clang-format on
#endif
}

ALWAYS_INLINE static bool atomic_dw_cas(uint64_t atomic[2], uint64_t expected[2], const uint64_t desired[2]) noexcept
{
	bool exchanged;
	asm volatile  // clang-format off
	(
		"lock cmpxchg16b (%[ptr])"
		: 		"=@ccz"	(exchanged),	// flag z is output
				"+a"	(expected[0]),	// rax: input/output
				"+d"	(expected[1])	// rdx: input/output
		:		"b"		(desired[0]),	// rbx: input only
				"c"		(desired[1]),	// rcx: input only
		  [ptr]	"r"		(atomic)
		:		"memory"				// "memory" acts as compiler r/w barrier
	);	// clang-format on
	return exchanged;
}

#endif

ALWAYS_INLINE static void atomic_dw_load(uint32_t atomic[2], uint32_t destination[2]) noexcept
{
#if defined(__x86_64__)
	asm volatile  // clang-format off
	(
		"movq (%[ptr]), %%rax\n\t"			// guaranteed to be atomic
		"shldq $32,%%rax,%%rdx"				// result is stored in eax:edx
		:		"=a"	(destination[0]),	// eax: output only
				"=d"	(destination[1])	// edx: output only
		: [ptr]	"r"		(atomic)
		:		"cc", "memory"				// flags modified. "memory" acts as compiler r/w barrier
	);	// clang-format on
#elif defined(__i686__)
	asm volatile  // clang-format off
	(
		"movl %%ebx, %%eax\n\t"		// eax:edx is early clobber and overwritten before cmpxchg16b
		"movl %%ecx, %%edx\n\t"
		"lock cmpxchg8b (%[ptr])"
		:		"=&a"	(reinterpret_cast<uint32_t*>(destination)[0]),	// eax: output only, early clobber
				"=&d"	(reinterpret_cast<uint32_t*>(destination)[1])	// edx: output only, early clobber
		: [ptr]	"r"		(atomic)
		:		"cc", "memory"				// flags modified. "memory" acts as compiler r/w barrier
	);	// clang-format on
#endif
}

ALWAYS_INLINE static void atomic_dw_store(uint32_t atomic[2], const uint32_t desired[2]) noexcept
{
#if defined(__x86_64__)
	[[maybe_unused]] uint32_t unused1;	// will be discarded
	[[maybe_unused]] uint32_t unused2;	// will be discarded
	asm volatile  // clang-format off
	(
		"shlq $32,%%rbx\n\t"			// data is stored in ebx:ecx
		"shldq $32,%%rbx,%%rcx\n\t"
		"movq %%rcx, (%[ptr])"			// guaranteed to be atomic
		:		"=b" (unused1),			// rbx is modified, discard it
				"=c" (unused2)			// rcx is modified, discard it
		:		"b" (desired[0]),
				"c" (desired[1]),
		  [ptr]	"r" (atomic)
		:		"cc", "memory"			// flags modified, "memory" acts as compiler r/w barrier
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
		:		"b" (reinterpret_cast<const uint32_t*>(desired)[0]),
				"c" (reinterpret_cast<const uint32_t*>(desired)[1]),
		  [ptr]	"r" (atomic)
		:		"cc", "eax", "edx", "memory"	// flags & eax:edx modified, "memory" acts as compiler r/w barrier
	);	// clang-format on
#endif
}

ALWAYS_INLINE static bool atomic_dw_cas(uint32_t atomic[2], uint32_t expected[2], const uint32_t desired[2])
{
	bool exchanged;
	asm volatile  // clang-format off
	(
		"lock cmpxchg8b (%[ptr])"
		:		"=@ccz"	(exchanged),	// flag z is output
				"+a"	(expected[0]),	// eax: input/output
				"+d"	(expected[1])	// edx: input/output
		:		"b"		(desired[0]),	// ebx: input only
				"c"		(desired[1]),	// ecx: input only
		  [ptr]	"r"		(atomic)
		:		"memory"				// "memory" acts as compiler r/w barrier
	);	// clang-format on
	return exchanged;
}

}  // namespace ConcurrentFW

#endif	// __x86_64__ || __i686__

#endif	// CONCURRENTFW_ATOMIC_ASM_X86_HPP_
