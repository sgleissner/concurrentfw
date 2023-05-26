/*
 * concurrentfw/atomic_asm_dwcas_llsc.hpp
 *
 * (C) 2017-2023 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the MIT license, see file LICENSE.
 */

#pragma once
#ifndef CONCURRENTFW_ATOMIC_ASM_DWCAS_LLSC_HPP_
#define CONCURRENTFW_ATOMIC_ASM_DWCAS_LLSC_HPP_

// the included files either define / provide the macros
// ATOMIC_DWCAS_NEEDED and atomic double-word load / store / compare and swap
// or
// ATOMIC_LLSC_NEEDED and atomic exclusive-load-aquire / exclusive-store-release
// inlined assembler code

#if defined __x86_64__ || defined __i686__
#include <concurrentfw/atomic_asm_x86.hpp>
#elif defined __arm__ || defined __aarch64__
#include <concurrentfw/atomic_asm_arm.hpp>
#else
#error "unsupported platform"
#endif

// we want the information about platform ABA helper also as C++ constexpr

namespace ConcurrentFW
{
enum class PlatformABASolution : bool
{
    DWCAS = false,
    LLSC = true
};

#if defined(ATOMIC_DWCAS_NEEDED)
constexpr static ConcurrentFW::PlatformABASolution PLATFORM_ABA_SOLUTION {ConcurrentFW::PlatformABASolution::DWCAS};
#elif defined(ATOMIC_LLSC_NEEDED)
constexpr static ConcurrentFW::PlatformABASolution PLATFORM_ABA_SOLUTION {ConcurrentFW::PlatformABASolution::LLSC};
#else
#error "unsupported platform: neither DWCAS nor LLSC supported"
#endif
}  // namespace ConcurrentFW

#endif  // CONCURRENTFW_ATOMIC_ASM_DWCAS_LLSC_HPP_
