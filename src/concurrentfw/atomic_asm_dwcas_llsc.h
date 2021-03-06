/*
 * concurrentfw/atomic_asm_dwcas_llsc.h
 *
 * (C) 2017-2020 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */


#ifndef CONCURRENTFW_ATOMIC_ASM_DWCAS_LLSC_H_
#define CONCURRENTFW_ATOMIC_ASM_DWCAS_LLSC_H_

// the included files either define / provide
// ATOMIC_DWCAS_NEEDED and atomic double-word load / store / compare and swap
// or
// ATOMIC_LLSC_NEEDED and atomic exclusive-load-aquire / exclusive-store-release
// inlined assembler code

#if defined __x86_64__ || defined __i686__
#include <concurrentfw/atomic_asm_x86.h>
#elif defined __arm__ || defined __aarch64__
#include <concurrentfw/atomic_asm_arm.h>
#else
#error "unsupported platform"
#endif

// we want the information about platform ABA helper also as C++ constexpr

namespace ConcurrentFW
{
	enum class PlatformABASolution : bool
	{
		DWCAS=false,
		LLSC=true
	};

#if defined(ATOMIC_DWCAS_NEEDED)
	constexpr static ConcurrentFW::PlatformABASolution PLATFORM_ABA_SOLUTION { ConcurrentFW::PlatformABASolution::DWCAS };
#elif defined(ATOMIC_LLSC_NEEDED)
	constexpr static ConcurrentFW::PlatformABASolution PLATFORM_ABA_SOLUTION { ConcurrentFW::PlatformABASolution::LLSC };
#else
#error "unsupported platform: neither DWCAS nor LLSC supported"
#endif
}

#endif	// CONCURRENTFW_ATOMIC_ASM_DWCAS_LLSC_H_
