/*
 * concurrentfw/helper.h
 *
 * (C) 2017-2020 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */


#ifndef CONCURRENTFW_HELPER_H_
#define CONCURRENTFW_HELPER_H_

// branch hint macros
// see: https://stackoverflow.com/questions/30130930/is-there-a-compiler-hint-for-gcc-to-force-branch-prediction-to-always-go-a-certa
// in c++20: use [[likely]] and [[unlikely]]
#define UNLIKELY(cond) __builtin_expect(static_cast<bool>(cond), false)
#define   LIKELY(cond) __builtin_expect(static_cast<bool>(cond), true)

// force inline code
#define ALWAYS_INLINE inline __attribute__((always_inline))

// count leading zeroes
#define CLZ(x) __builtin_clz(x)		/* x must not be 0 */


namespace ConcurrentFW
{

enum class PlatformWidth : unsigned char
{
	WIDTH_32 = 32,
	WIDTH_64 = 64
};

// platform helpers
#if defined(__x86_64__) || defined(__aarch64__)
#define PLATFORM_WIDTH_64
constexpr PlatformWidth PLATFORM_WIDTH { PlatformWidth::WIDTH_64 };
#elif defined(__i686__) || defined(__arm__)
#define PLATFORM_WIDTH_32
constexpr PlatformWidth PLATFORM_WIDTH { PlatformWidth::WIDTH_32 };
#else
#error "unsupported platform"
#endif

} // namespace ConcurrentFW

#endif /* CONCURRENTFW_HELPER_H_ */
