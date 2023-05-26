/*
 * concurrentfw/aba_wrapper.h
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

// for a description of the ABA problem, please see: https://en.wikipedia.org/wiki/ABA_problem

#pragma once
#ifndef CONCURRENTFW_ABA_WRAPPER_HPP_
#define CONCURRENTFW_ABA_WRAPPER_HPP_

#include <cstdint>
#include <type_traits>
#include <cstddef>	// offsetof()

#include <concurrentfw/atomic_asm_dwcas_llsc.hpp>
#include <concurrentfw/helper.hpp>


#define ATTRIBUTE_ABA_LOOP_OPTIMIZE \
	gnu::optimize(GNU_OPTIMIZE_ATOMIC_LOOPS_ALIGNMENT, "no-split-loops", "no-unswitch-loops")

namespace ConcurrentFW
{

static constexpr bool ABA_IS_PLATFORM_64 {(ConcurrentFW::PLATFORM_WIDTH == ConcurrentFW::PlatformWidth::WIDTH_64)};
static constexpr bool ABA_IS_PLATFORM_DWCAS {
	(ConcurrentFW::PLATFORM_ABA_SOLUTION == ConcurrentFW::PlatformABASolution::DWCAS)};

static constexpr size_t ABA_MAX_DATA_SIZE {ABA_IS_PLATFORM_64 ? 8 : 4};
static constexpr size_t ABA_ARRAY_SIZE {ABA_IS_PLATFORM_DWCAS ? 2 : 1};	 // DWCAS: additional word for counter

template<typename T>
static constexpr size_t ABA_ATOMIC_ALIGNMENT {ABA_ARRAY_SIZE * (ABA_IS_PLATFORM_64 && sizeof(T) > 4 ? 8 : 4)};

template<typename T>
using Atomic_ABA_BaseType = typename std::conditional_t<(ABA_IS_PLATFORM_64 && sizeof(T) > 4), uint64_t, uint32_t>;


// If we want to implement an ABA protection for a given type T,
// this class helps us to wrap T architecture-independent ABA-safe.
// T must be a simple integer or pointer type.
// In case of using DWCAS and the need of an internal change counter,
// we require it to be at least 32 bit wide.

template<typename T>
union alignas(ABA_ATOMIC_ALIGNMENT<T>) ABA_Wrapper
{
	// This helper class works only with simple integral or pointer types.
	static_assert(std::is_pointer_v<T> || std::is_integral_v<T>, "T is neither pointer, nor integral type.");
	static_assert(sizeof(T) <= ABA_MAX_DATA_SIZE, "size of T too big.");
	static_assert(sizeof(T) >= sizeof(uint32_t), "size of T too small.");
	static_assert((sizeof(T) == sizeof(uint32_t)) || (sizeof(T) == ABA_MAX_DATA_SIZE), "size of T does not match");

private:
	union WrapperContent  // union is per default uninitialized
	{
		Atomic_ABA_BaseType<T> atomic[ABA_ARRAY_SIZE];
		T data;
	};

	static_assert(offsetof(WrapperContent, atomic[0]) == 0, "internal atomic offset mismatch");
	static_assert(offsetof(WrapperContent, data) == 0, "internal data offset mismatch");
	static_assert(sizeof(WrapperContent::atomic[0]) == sizeof(WrapperContent::data), "internal content size mismatch");

public:
	using Counter = Atomic_ABA_BaseType<T>;	 // only used on DWCAS plattforms

	static constexpr size_t alignment {ABA_ATOMIC_ALIGNMENT<T>};

private:
	WrapperContent content;

public:
	explicit ABA_Wrapper(T init)
	{
		WrapperContent wrapper_init;
		wrapper_init.data = init;
		if constexpr (ABA_IS_PLATFORM_DWCAS)
		{
			wrapper_init.atomic[1] = 1;	 // initialize counter
			atomic_dw_store(content.atomic, wrapper_init.atomic);
		}
		else  // constexpr(LLSC)
		{
			compiler_barrier();
			__atomic_store_n(&content.atomic[0], wrapper_init.atomic[0], __ATOMIC_RELEASE);
			compiler_barrier();
		}
	}

	Counter get_counter()  // for testing only
	{
		if constexpr (ABA_IS_PLATFORM_DWCAS)
		{
			WrapperContent cache;
			atomic_dw_load(content.atomic, cache.atomic);
			return cache.atomic[1];
		}
		else  // constexpr(LLSC)
		{
			return 0;  // no counter needed for LLSC
		}
	}

	T get()
	{
		WrapperContent cache;
		if constexpr (ABA_IS_PLATFORM_DWCAS)
		{
			atomic_dw_load(content.atomic, cache.atomic);
		}
		else  // constexpr(LLSC)
		{
			compiler_barrier();
			cache.atomic[0] = __atomic_load_n(&content.atomic[0], __ATOMIC_ACQUIRE);
			compiler_barrier();
		}
		return cache.data;
	}

	// align loop to 64 byte on LLSC platforms (try to keep LL/SC in one cache line)
	// align loops to 1 byte on Intel platform (as loop is UNLIKELY)
	// https://stackoverflow.com/questions/7281699/aligning-to-cache-line-and-knowing-the-cache-line-size

	template<typename... ARGS>
	inline bool modify [[gnu::always_inline, ATTRIBUTE_ABA_LOOP_OPTIMIZE]] (auto modifier_func)
	{
		bool success;
		bool stored;

		if constexpr (ABA_IS_PLATFORM_DWCAS)
		{
			WrapperContent cache;
			atomic_dw_load(content.atomic, cache.atomic);

			do
			{
				WrapperContent desired;

				success = modifier_func(cache.data, desired.data);	// will be inlined
				if (!success) [[unlikely]]
					break;

				desired.atomic[1] = cache.atomic[1] + 1;
				stored = atomic_dw_cas(content.atomic, cache.atomic, desired.atomic);
			}
			while (UNLIKELY(!stored));
		}
		else  // constexpr(LLSC)
		{
			do
			{
				WrapperContent desired;
				WrapperContent cache;
				atomic_exclusive_load_aquire(content.atomic[0], cache.atomic[0]);

				success = modifier_func(cache.data, desired.data);	// will be inlined
				if (!success) [[unlikely]]
				{
					atomic_exclusive_abort(content.atomic[0]);
					break;
				}

				stored = atomic_exclusive_store_release(content.atomic[0], desired.atomic[0]);
			}
			while (UNLIKELY(!stored));
		}

		return success;
	}
};

}  // namespace ConcurrentFW

#endif /* CONCURRENTFW_ABA_WRAPPER_HPP_ */
