/*
 * concurrentfw/atomic.h
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */


// ConcurrentFW::Atomic
// Purpose: gcc-atomic-builtin wrapper, similar to std::atomic,
// but with direct access to the atomic value to be usable in futexes and other constructs
// and template-coded memory order (which is always constant and will be verified)
// see: https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html

#pragma once
#ifndef CONCURRENTFW_ATOMIC_HPP_
#define CONCURRENTFW_ATOMIC_HPP_

#include <type_traits>

#include <concurrentfw/helper.hpp>

namespace ConcurrentFW
{


//////////////////////////////////////////////////////////////////////////
// possible memory orders (as enum)
//////////////////////////////////////////////////////////////////////////

enum class AtomicMemoryOrder : int
{
	RELAXED = __ATOMIC_RELAXED,
	CONSUME = __ATOMIC_CONSUME,
	ACQUIRE = __ATOMIC_ACQUIRE,
	RELEASE = __ATOMIC_RELEASE,
	ACQ_REL = __ATOMIC_ACQ_REL,
	SEQ_CST = __ATOMIC_SEQ_CST
};

static consteval int memorder(AtomicMemoryOrder atomic_memory_order)
{
	return static_cast<int>(atomic_memory_order);
}

//////////////////////////////////////////////////////////////////////////
// allowed memory orders (as concepts)
//////////////////////////////////////////////////////////////////////////

template<AtomicMemoryOrder MEMORDER>
concept AtomicMemoryOrderAcquireConsume
	= (MEMORDER == AtomicMemoryOrder::ACQUIRE) || (MEMORDER == AtomicMemoryOrder::CONSUME);

template<AtomicMemoryOrder MEMORDER>
concept AtomicMemoryOrderLoad = (MEMORDER == AtomicMemoryOrder::RELAXED) || (MEMORDER == AtomicMemoryOrder::SEQ_CST)
								|| (MEMORDER == AtomicMemoryOrder::ACQUIRE) || (MEMORDER == AtomicMemoryOrder::CONSUME);

template<AtomicMemoryOrder MEMORDER>
concept AtomicMemoryOrderStore = (MEMORDER == AtomicMemoryOrder::RELAXED) || (MEMORDER == AtomicMemoryOrder::SEQ_CST)
								 || (MEMORDER == AtomicMemoryOrder::RELEASE);

template<AtomicMemoryOrder MEMORDER_SUCCESS, AtomicMemoryOrder MEMORDER_FAILURE>
concept AtomicMemoryOrderCompareExchange
	= (MEMORDER_FAILURE == AtomicMemoryOrder::RELAXED)
	  || ((MEMORDER_FAILURE == AtomicMemoryOrder::CONSUME)
		  && ((MEMORDER_SUCCESS == AtomicMemoryOrder::SEQ_CST) || (MEMORDER_SUCCESS == AtomicMemoryOrder::ACQUIRE)
			  || (MEMORDER_SUCCESS == AtomicMemoryOrder::ACQ_REL) || (MEMORDER_SUCCESS == AtomicMemoryOrder::RELEASE)
			  || (MEMORDER_SUCCESS == AtomicMemoryOrder::CONSUME)))
	  || ((MEMORDER_FAILURE == AtomicMemoryOrder::ACQUIRE)
		  && ((MEMORDER_SUCCESS == AtomicMemoryOrder::SEQ_CST) || (MEMORDER_SUCCESS == AtomicMemoryOrder::ACQUIRE)
			  || (MEMORDER_SUCCESS == AtomicMemoryOrder::ACQ_REL) || (MEMORDER_SUCCESS == AtomicMemoryOrder::RELEASE)))
	  || ((MEMORDER_FAILURE == AtomicMemoryOrder::SEQ_CST) && (MEMORDER_SUCCESS == AtomicMemoryOrder::SEQ_CST));

//////////////////////////////////////////////////////////////////////////
// allowed types for atomics (as concepts)
//////////////////////////////////////////////////////////////////////////

template<typename T>
concept AtomicBaseType = std::is_integral_v<T> || std::is_pointer_v<T>;

//////////////////////////////////////////////////////////////////////////
// Atomic type (as public fully accessible struct)
//////////////////////////////////////////////////////////////////////////

template<AtomicBaseType T>
struct alignas(sizeof(T)) Atomic
{
	static_assert(std::is_integral<T>() || std::is_pointer<T>(), "<T> must be integral type or pointer");
	static_assert(__atomic_always_lock_free(sizeof(T), nullptr), "built-in atomic is not always lock free");

	//////////////////////////////////////////////////////////////////////////
	// atomic value
	//////////////////////////////////////////////////////////////////////////

	volatile T atomic;	// NOSONAR accessible from outside

	//////////////////////////////////////////////////////////////////////////
	// constructors
	//////////////////////////////////////////////////////////////////////////

	Atomic() = default;	 // atomic is uninitialized

	Atomic(const T value) noexcept
	{
		store<AtomicMemoryOrder::RELAXED>(value);
	}

	~Atomic() = default;

	//////////////////////////////////////////////////////////////////////////
	// load, store
	//////////////////////////////////////////////////////////////////////////

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
		requires AtomicMemoryOrderLoad<MEMORDER>
	ALWAYS_INLINE T load() const noexcept
	{
		compiler_barrier();
		T retval = __atomic_load_n(&atomic, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
		requires AtomicMemoryOrderStore<MEMORDER>
	ALWAYS_INLINE void store(const T store) noexcept
	{
		compiler_barrier();
		__atomic_store_n(&atomic, store, memorder(MEMORDER));
		compiler_barrier();
	}

	//////////////////////////////////////////////////////////////////////////
	// exchange, compare_exchange
	//////////////////////////////////////////////////////////////////////////

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T exchange(const T exchange) noexcept
	{
		compiler_barrier();
		T retval = __atomic_exchange_n(&atomic, exchange, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<
		AtomicMemoryOrder MEMORDER_SUCCESS = AtomicMemoryOrder::SEQ_CST,
		AtomicMemoryOrder MEMORDER_FAILURE = AtomicMemoryOrder::SEQ_CST>
		requires AtomicMemoryOrderCompareExchange<MEMORDER_SUCCESS, MEMORDER_FAILURE>
	ALWAYS_INLINE bool compare_exchange_weak(T& expected, const T desired) noexcept
	{
		compiler_barrier();
		bool retval = __atomic_compare_exchange_n(
			&atomic, &expected, desired, true /* weak */, memorder(MEMORDER_SUCCESS), memorder(MEMORDER_FAILURE)
		);
		compiler_barrier();
		return retval;
	}

	template<
		AtomicMemoryOrder MEMORDER_SUCCESS = AtomicMemoryOrder::SEQ_CST,
		AtomicMemoryOrder MEMORDER_FAILURE = AtomicMemoryOrder::SEQ_CST>
		requires AtomicMemoryOrderCompareExchange<MEMORDER_SUCCESS, MEMORDER_FAILURE>
	ALWAYS_INLINE bool compare_exchange_strong(T& expected, const T desired) noexcept
	{
		compiler_barrier();
		bool retval = __atomic_compare_exchange_n(
			&atomic, &expected, desired, false /* strong */, memorder(MEMORDER_SUCCESS), memorder(MEMORDER_FAILURE)
		);
		compiler_barrier();
		return retval;
	}

	//////////////////////////////////////////////////////////////////////////
	// fetch & modify
	//////////////////////////////////////////////////////////////////////////

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T add_fetch(const T value) noexcept
	{
		compiler_barrier();
		T retval = __atomic_add_fetch(&atomic, value, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T fetch_add(const T value) noexcept
	{
		compiler_barrier();
		T retval = __atomic_fetch_add(&atomic, value, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T sub_fetch(const T value) noexcept
	{
		compiler_barrier();
		T retval = __atomic_sub_fetch(&atomic, value, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T fetch_sub(const T value) noexcept
	{
		compiler_barrier();
		T retval = __atomic_fetch_sub(&atomic, value, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T and_fetch(const T value) noexcept
	{
		compiler_barrier();
		T retval = __atomic_and_fetch(&atomic, value, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T fetch_and(const T value) noexcept
	{
		compiler_barrier();
		T retval = __atomic_fetch_and(&atomic, value, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T xor_fetch(const T value) noexcept
	{
		compiler_barrier();
		T retval = __atomic_xor_fetch(&atomic, value, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T fetch_xor(const T value) noexcept
	{
		compiler_barrier();
		T retval = __atomic_fetch_xor(&atomic, value, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T or_fetch(const T value) noexcept
	{
		compiler_barrier();
		T retval = __atomic_or_fetch(&atomic, value, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T fetch_or(const T value) noexcept
	{
		compiler_barrier();
		T retval = __atomic_fetch_or(&atomic, value, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T nand_fetch(const T value) noexcept
	{
		compiler_barrier();
		T retval = __atomic_nand_fetch(&atomic, value, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE T fetch_nand(const T value) noexcept
	{
		compiler_barrier();
		T retval = __atomic_fetch_nand(&atomic, value, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	//////////////////////////////////////////////////////////////////////////
	// byte operations
	//////////////////////////////////////////////////////////////////////////

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
	ALWAYS_INLINE bool test_and_set() noexcept
	{
		static_assert(sizeof(T) == 1, "<T> shall only be char or bool with test_and_set()");
		compiler_barrier();
		T retval = __atomic_test_and_set(&atomic, memorder(MEMORDER));
		compiler_barrier();
		return retval;
	}

	template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
		requires AtomicMemoryOrderStore<MEMORDER>
	ALWAYS_INLINE void clear() noexcept
	{
		static_assert(sizeof(T) == 1, "<T> shall only be char or bool with clear()");
		compiler_barrier();
		__atomic_clear(&atomic, memorder(MEMORDER));
		compiler_barrier();
	}
};

//////////////////////////////////////////////////////////////////////////
// fence operations
//////////////////////////////////////////////////////////////////////////

template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
static ALWAYS_INLINE void atomic_thread_fence() noexcept
{
	compiler_barrier();
	__atomic_thread_fence(memorder(MEMORDER));
	compiler_barrier();
}

template<AtomicMemoryOrder MEMORDER = AtomicMemoryOrder::SEQ_CST>
static ALWAYS_INLINE void atomic_signal_fence() noexcept
{
	compiler_barrier();
	__atomic_signal_fence(memorder(MEMORDER));
	compiler_barrier();
}

}  // namespace ConcurrentFW

#endif	// CONCURRENTFW_ATOMIC_HPP_
