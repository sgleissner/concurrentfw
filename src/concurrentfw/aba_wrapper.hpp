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
#include <cstddef>

#include <concurrentfw/atomic_asm_dwcas_llsc.hpp>
#include <concurrentfw/helper.hpp>


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
using Atomic_ABA_BaseType = typename std::conditional<(ABA_IS_PLATFORM_64 && sizeof(T) > 4), uint64_t, uint32_t>::type;


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
	static_assert(sizeof(T) >= sizeof(uint32_t), "size of T too small.");  // TODO: may be removed later

public:
	using Counter = Atomic_ABA_BaseType<T>;

private:
	Atomic_ABA_BaseType<T> atomic[ABA_ARRAY_SIZE];
	T data;

public:
	ABA_Wrapper() = default;  // union is per default uninitialized
	ABA_Wrapper(T init)
	: data {init}
	{
		// if DWCAS is used, initialize counter
		if constexpr (ABA_IS_PLATFORM_DWCAS)
			atomic[1] = 1;
	}

	Counter get_counter()  // for testing only
	{
		if constexpr (ABA_IS_PLATFORM_DWCAS)
		{
			ABA_Wrapper cache;
			atomic_dw_load(atomic, cache.atomic);
			return cache.atomic[1];
		}
		else  // LLSC
		{
			return 0;  // no counter needed for LLSC
		}
	}

	T get_nonatomic()  // cave
	{
		return data;
	}

	void set_nonatomic(const T& set)  // cave
	{
		data = set;
	}

	T get()
	{
		if constexpr (ABA_IS_PLATFORM_DWCAS)
		{
			ABA_Wrapper cache;
			atomic_dw_load(atomic, cache.atomic);
			return cache.data;
		}
		else  // LLSC
		{
			return data;  // TODO atomic read
		}
	}

	// align loop to 64 byte on LLSC platforms
	// align loops to 1 byte on Intel platform (as loop is UNLIKELY)
	// https://stackoverflow.com/questions/7281699/aligning-to-cache-line-and-knowing-the-cache-line-size

	template<typename... ARGS>
	inline bool modify
		[[gnu::always_inline,
		  gnu::optimize(GNU_OPTIMIZE_ATOMIC_LOOPS_ALIGNMENT
		  )]] (bool (*modifier_func)(const T&, T&, ARGS...), ARGS... args)
	{
		bool success;
		bool stored;

		if constexpr (ABA_IS_PLATFORM_DWCAS)
		{
			ABA_Wrapper cache;
			atomic_dw_load(atomic, cache.atomic);

			do
			{
				ABA_Wrapper desired;

				success = modifier_func(cache.data, desired.data, args...);	 // will be inlined
				if (!success) [[unlikely]]
					break;

				desired.atomic[1] = cache.atomic[1] + 1;
				stored = atomic_dw_cas(atomic, cache.atomic, desired.atomic);
			}
			while (UNLIKELY(!stored));
		}
		else  // constexpr(LLSC)
		{
			do
			{
				ABA_Wrapper desired;
				ABA_Wrapper cache;
				atomic_exclusive_load_aquire(atomic[0], cache.atomic[0]);

				success = modifier_func(cache.data, desired.data, args...);	 // will be inlined
				if (!success) [[unlikely]]
				{
					atomic_exclusive_abort(atomic[0]);
					break;
				}

				stored = atomic_exclusive_store_release(atomic[0], desired.atomic[0]);
			}
			while (UNLIKELY(!stored));
		}

		return success;
	}
};

}  // namespace ConcurrentFW

#endif /* CONCURRENTFW_ABA_WRAPPER_HPP_ */
