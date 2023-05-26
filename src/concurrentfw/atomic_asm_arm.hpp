/*
 * concurrentfw/atomic_asm_arm.hpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the MIT license, see file LICENSE.
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

#if __ARM_ARCH < 7  // minimum ARMv7, shall be ARMv6K in the future
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

inline static void atomic_exclusive_abort(uint64_t& /*atomic*/)
{
    asm volatile("clrex");  // clear exclusive monitor
}

#endif

inline static void atomic_exclusive_abort(uint32_t& /*atomic*/)
{
    asm volatile("clrex");  // clear exclusive monitor
}

#if defined(__aarch64__)

// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0802a/LDXR.html

// ldaxr documentation
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100076_0100_00_en/pge1427897401219.html

inline static void atomic_exclusive_load_aquire(uint64_t& atomic, uint64_t& transfer)
{
    asm volatile("ldaxr %0, [%1]"  // load-acquire exclusive register
                 : "=r"(transfer)  // transfer register
                 : "r"(&atomic)    // atomic base register
                 : "memory");      // "memory" acts as compiler r/w barrier
}

inline static void atomic_exclusive_load_aquire(uint32_t& atomic, uint32_t& transfer)
{
    asm volatile("ldaxr %w0, [%1]"  // load-acquire exclusive register
                 : "=r"(transfer)   // transfer register
                 : "r"(&atomic)     // atomic base register
                 : "memory");       // "memory" acts as compiler r/w barrier
}

// ldaxp documentation
// https://developer.arm.com/documentation/dui0802/b/A64-Data-Transfer-Instructions/LDAXP

inline static void atomic_exclusive_load_pair_aquire(uint64_t atomic[2], uint64_t transfer[2])
{
    asm volatile("ldaxp %0, %1, [%2]"  // load-acquire exclusive register pair
                 : "=r"(transfer[0]),  // first transfer register
                   "=r"(transfer[1])   // second transfer register
                 : "r"(&atomic[0])     // atomic base register
                 : "memory");          // "memory" acts as compiler r/w barrier
}

inline static void atomic_exclusive_load_pair_aquire(uint32_t atomic[2], uint32_t transfer[2])
{
    asm volatile("ldaxp %w0, %w1, [%2]"  // load-acquire exclusive register pair
                 : "=r"(transfer[0]),    // first transfer register
                   "=r"(transfer[1])     // second transfer register
                 : "r"(&atomic[0])       // atomic base register
                 : "memory");            // "memory" acts as compiler r/w barrier
}

// stlxr documentation
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100076_0100_00_en/pge1427897423561.html

inline static bool atomic_exclusive_store_release(uint64_t& atomic, const uint64_t& transfer)
{
    uint32_t failed;                    // native size of result
    asm volatile("stlxr %w0, %1, [%2]"  // store-release exclusive register, returning status
                 : "=&r"(failed)   // store result, early clobber: prevent double usage with transfer/base registers
                 : "r"(transfer),  // transfer register
                   "r"(&atomic)    // atomic base register
                 : "memory");      // "memory" acts as compiler r/w barrier
    return (failed != 0);
}

inline static bool atomic_exclusive_store_release(uint32_t& atomic, const uint32_t& transfer)
{
    uint32_t failed;                     // native size of result
    asm volatile("stlxr %w0, %w1, [%2]"  // store-release exclusive register, returning status
                 : "=&r"(failed)   // store result, early clobber: prevent double usage with transfer/base registers
                 : "r"(transfer),  // transfer register
                   "r"(&atomic)    // atomic base register
                 : "memory");      // "memory" acts as compiler r/w barrier
    return (failed != 0);
}

// stlxp documentation
// https://developer.arm.com/documentation/dui0802/b/A64-Data-Transfer-Instructions/STLXP

inline static bool atomic_exclusive_store_pair_release(uint64_t atomic[2], const uint64_t[2] transfer)
{
    uint32_t failed;                        // native size of result
    asm volatile("stlxp %w0, %1, %2, [%3]"  // store-release exclusive register pair, returning status
                 : "=&r"(failed)      // store result, early clobber: prevent double usage with transfer/base registers
                 : "r"(transfer[0]),  // first transfer register
                   "r"(transfer[1]),  // second transfer register
                   "r"(&atomic[0])    // atomic base register
                 : "memory");         // "memory" acts as compiler r/w barrier
    return (failed != 0);
}

inline static bool atomic_exclusive_store_pair_release(uint32_t atomic[2], const uint32_t transfer[2])
{
    uint32_t failed;                          // native size of result
    asm volatile("stlxp %w0, %w1, %w2, [%3]"  // store-release exclusive register pair, returning status
                 : "=&r"(failed)      // store result, early clobber: prevent double usage with transfer/base registers
                 : "r"(transfer[0]),  // first transfer register
                   "r"(transfer[1]),  // second transfer register
                   "r"(&atomic[0])    // atomic base register
                 : "memory");         // "memory" acts as compiler r/w barrier
    return (failed != 0);
}

#elif defined(__arm__)

#if __ARM_ARCH >= 8  // ARMv8 and higher

inline static void atomic_exclusive_load_aquire(uint32_t& atomic, uint32_t& transfer)
{
    asm volatile("ldaex %0, [%1]"  // load-acquire exclusive register
                 : "=r"(transfer)  // transfer register
                 : "r"(&atomic)    // atomic base register
                 : "memory");      // "memory" acts as compiler r/w barrier
}

inline static bool atomic_exclusive_store_release(uint32_t& atomic, const uint32_t& transfer)
{
    uint32_t failed;                   // native size of result
    asm volatile("stlex %0, %1, [%2]"  // store-release exclusive register, returning status
                 : "=&r"(failed)       // store result, early clobber: prevent double usage with transfer/base registers
                 : "r"(transfer),      // transfer register
                   "r"(&atomic)        // atomic base register
                 : "memory");          // "memory" acts as compiler r/w barrier
    return (failed != 0);
}

// register pairs ARMv8 32bit
// https://developer.arm.com/documentation/dui0802/b/A32-and-T32-Instructions/LDAEX-and-STLEX
// TODO: For LDAEXD and STLEXD, Rt must be an even numbered register, and not LR.
// TODO: Rt2 must be R(t+1).

inline static void atomic_exclusive_load_pair_aquire(uint32_t atomic[2], uint32_t transfer[2])
{
    uint64_t pair;
    asm volatile("ldaexd %Q0, %R0, [%1]"  // load-acquire exclusive register pair
                 : "=r"(pair)             // transfer register pair
                 : "r"(&atomic[0])        // atomic base register
                 : "memory");             // "memory" acts as compiler r/w barrier
    transfer[0] = static_cast<uint32_t>(pair);
    transfer[1] = static_cast<uint32_t>(pair >> 32);
}

inline static bool atomic_exclusive_store_pair_release(uint32_t atomic[2], const uint32_t transfer[2])
{
    const uint64_t pair = static_cast<const uint64_t>(transfer[0]) | (static_cast<const uint64_t>(transfer[1]) << 32);
    uint32_t failed;                          // native size of result
    asm volatile("stlexd %0, %Q1, %R1, [%2]"  // store-release exclusive register pair
                 : "=&r"(failed)    // store result, early clobber: prevent double usage with transfer/base registers
                 : "r"(pair),       // transfer register pair
                   "r"(&atomic[0])  // atomic base register
                 : "memory");       // "memory" acts as compiler r/w barrier
    return (failed != 0);
}

#elif __ARM_ARCH >= 7  // ARMv7

inline static void atomic_exclusive_load_aquire(uint32_t& atomic, uint32_t& transfer)
{
    asm volatile("ldrex %0, [%1]\n\t "  // load exclusive register
                 "dmb"                  // full memory fence
                 : "=r"(transfer)       // transfer register
                 : "r"(&atomic)         // atomic base register
                 : "memory");           // "memory" acts as compiler r/w barrier
}

inline static bool atomic_exclusive_store_release(uint32_t& atomic, const uint32_t& transfer)
{
    uint32_t failed;                   // native size of result
    asm volatile("dmb\n\t"             // full memory fence
                 "strex %0, %1, [%2]"  // store exclusive register, returning status
                 : "=&r"(failed)       // store result, early clobber: prevent double usage with transfer/base registers
                 : "r"(transfer),      // transfer register
                   "r"(&atomic)        // atomic base register
                 : "memory");          // "memory" acts as compiler r/w barrier
    return (failed != 0);
}

// register pairs ARMv7
// https://developer.arm.com/documentation/dui0802/b/A32-and-T32-Instructions/LDREX-and-STREX
// TODO: For LDREXD and STREXD, Rt must be an even numbered register, and not LR.
// TODO: Rt2 must be R(t+1).

inline static void atomic_exclusive_load_pair_aquire(uint32_t atomic[2], uint32_t transfer[2])
{
    uint64_t pair;
    asm volatile("ldrexd %Q0, %R0, [%1]\n\t "  // load exclusive register pair
                 "dmb"                         // full memory fence
                 : "=r"(pair)                  // transfer register pair
                 : "r"(&atomic[0])             // atomic base register
                 : "memory");                  // "memory" acts as compiler r/w barrier
    transfer[0] = static_cast<uint32_t>(pair);
    transfer[1] = static_cast<uint32_t>(pair >> 32);
}

inline static bool atomic_exclusive_store_pair_release(uint32_t atomic[2], const uint32_t transfer[2])
{
    const uint64_t pair = static_cast<const uint64_t>(transfer[0]) | (static_cast<const uint64_t>(transfer[1]) << 32);
    uint32_t failed;                          // native size of result
    asm volatile("dmb\n\t"                    // full memory fence
                 "strexd %0, %Q1, %R1, [%2]"  // store exclusive register pair, returning status
                 : "=&r"(failed)    // store result, early clobber: prevent double usage with transfer/base registers
                 : "r"(pair),       // transfer register pair
                   "r"(&atomic[0])  // atomic base register
                 : "memory");       // "memory" acts as compiler r/w barrier
    return (failed != 0);
}

#else  // ARMv6K

#error "memory barrier code still missing for ARMv6K"

#endif  // ARMv6K

#endif  // __arm__

}  // namespace ConcurrentFW

#endif  // __arm__ || defined __aarch64__

#endif  // CONCURRENTFW_ATOMIC_ASM_ARM_HPP_
