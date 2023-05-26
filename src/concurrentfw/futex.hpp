/*
 * concurrentfw/futex.h
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

/*
 * Fast Userspace Mutex
 *
 * futex algorithm based on work of Ulrich Drepper
 * see: http://www.akkadia.org/drepper/futex.pdf
 *
 * Implements private futexes for own threads only,
 * no interprocess futexes (and so no priority-inheritance futexes).
 */

#pragma once
#ifndef CONCURRENTFW_FUTEX_HPP_
#define CONCURRENTFW_FUTEX_HPP_

#include <unistd.h>		  // syscall()
#include <sys/syscall.h>  // SYS_futex
#include <linux/futex.h>  // constants for futex syscall

#include <cstdint>

#include <concurrentfw/helper.hpp>
#include <concurrentfw/atomic.hpp>


namespace ConcurrentFW
{

class FutexBase
{
public:
	enum class Op : uint8_t
	{
		SET = FUTEX_OP_SET,
		ADD = FUTEX_OP_ADD,
		OR = FUTEX_OP_OR,
		ANDN = FUTEX_OP_ANDN,
		XOR = FUTEX_OP_XOR
	};

	enum class OpArgShift : bool
	{
		DIRECT = false,
		SHIFT = true
	};

	enum class Cmp : uint8_t
	{
		EQ = FUTEX_OP_CMP_EQ,
		NE = FUTEX_OP_CMP_NE,
		LT = FUTEX_OP_CMP_LT,
		LE = FUTEX_OP_CMP_LE,
		GT = FUTEX_OP_CMP_GT,
		GE = FUTEX_OP_CMP_GE
	};

protected:
	explicit FutexBase(int init) noexcept
	: value(init)
	{}

	~FutexBase() = default;

public:
	FutexBase() = delete;
	FutexBase(const FutexBase&) = delete;
	FutexBase(FutexBase&&) = delete;
	FutexBase& operator=(const FutexBase&) = delete;
	FutexBase& operator=(FutexBase&&) = delete;

protected:
	static ALWAYS_INLINE long syscall_futex(
		volatile int* addr1, int op, int val1, const struct timespec* timeout, volatile int* addr2, int val3
	) noexcept
	{
		return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
	}

	static ALWAYS_INLINE long syscall_futex(
		volatile int* addr1, int op, int val1, uint32_t val2, volatile int* addr2, int val3
	) noexcept
	{
		return syscall(
			SYS_futex, addr1, op, val1, reinterpret_cast<void*>(static_cast<uintptr_t>(val2)), addr2, val3	// NOSONAR
		);
	}

	ALWAYS_INLINE int futex_wait(int expected, const struct timespec* timeout_relative = nullptr) noexcept
	{
		return static_cast<int>(syscall_futex(&value.atomic, FUTEX_WAIT_PRIVATE, expected, timeout_relative, nullptr, 0)
		);
	}

	ALWAYS_INLINE int futex_wake(int wakeups = 1) noexcept
	{
		return static_cast<int>(syscall_futex(&value.atomic, FUTEX_WAKE_PRIVATE, wakeups, nullptr, nullptr, 0));
	}

	// FUTEX_REQUEUE is "proved to be broken and unusable"

	ALWAYS_INLINE int futex_cmp_requeue(int wakeups, uint32_t limit, volatile int* target, int expected) noexcept
	{
		return static_cast<int>(
			syscall_futex(&value.atomic, FUTEX_CMP_REQUEUE_PRIVATE, wakeups, limit, target, expected)
		);
	}

	ALWAYS_INLINE int futex_wake_op(  // NOSONAR
		int wakeups1,
		uint32_t wakeups2,
		volatile int* address2,
		const Cmp cmp,
		const uint16_t cmparg,
		const Op op,
		const uint16_t oparg,
		const OpArgShift oparg_shift = OpArgShift::DIRECT
	) noexcept
	{
		const uint32_t val3 = (static_cast<uint32_t>(oparg_shift) << 31) | (static_cast<uint32_t>(op) << 28)
							  | (static_cast<uint32_t>(cmp) << 24) | (static_cast<uint32_t>(oparg & 0xFFF) << 12)
							  | (static_cast<uint32_t>(cmparg & 0xFFF));
		return static_cast<int>(
			syscall_futex(&value.atomic, FUTEX_WAKE_OP_PRIVATE, wakeups1, wakeups2, address2, static_cast<int>(val3))
		);
	}

	ALWAYS_INLINE int futex_wait_bitset(uint32_t mask, int expected, struct timespec* timeout_absolute) noexcept
	{
		return static_cast<int>(syscall_futex(
			&value.atomic, FUTEX_WAIT_BITSET_PRIVATE, expected, timeout_absolute, nullptr, static_cast<int>(mask)
		));
	}

	ALWAYS_INLINE int futex_wake_bitset(uint32_t mask, int wakeups = 1) noexcept
	{
		return static_cast<int>(
			syscall_futex(&value.atomic, FUTEX_WAKE_BITSET_PRIVATE, wakeups, nullptr, nullptr, static_cast<int>(mask))
		);
	}

protected:
	ConcurrentFW::Atomic<int> value;
};

class Futex : public FutexBase
{
private:
	enum State : int
	{
		UNLOCKED = 0,
		LOCKED_NOWAITERS = 1,
		LOCKED_WAITERS = 2
	};

public:
	Futex() noexcept
	: FutexBase(State::UNLOCKED)
	{}

	Futex(bool locked) noexcept
	: FutexBase(locked ? State::LOCKED_NOWAITERS : State::UNLOCKED)
	{}

	Futex(const Futex&) = delete;
	Futex(Futex&&) = delete;
	~Futex() = default;
	Futex& operator=(const Futex&) = delete;
	Futex& operator=(Futex&&) = delete;

	// TODO: Destructor: Test auf blockierte Futexes

	ALWAYS_INLINE void lock(void)
	{
		int expected_found = State::UNLOCKED;  // 0: expected value: unlocked
		// memory order: critical section below, therefore to an atomic-aquire here here (but only in case of success)
		if (value.compare_exchange_strong<AtomicMemoryOrder::ACQUIRE, AtomicMemoryOrder::RELAXED>(
				expected_found, State::LOCKED_NOWAITERS
			)
			== false) [[unlikely]]	// 1: desired value: locked, no other waiting
		{							// == false : is already locked, c contains found value
			wait(expected_found);	// wait for futex, value is now 1 (LOCKED_NOWAITERS) or 2 (LOCKED_WAITERS)
		}
	}

	ALWAYS_INLINE bool trylock(void) noexcept
	{
		int expected = State::UNLOCKED;	 // 0: expected value: unlocked
		// memory order: critical section below, therefore to an atomic-aquire here here (but only in case of success)
		return value.compare_exchange_strong<AtomicMemoryOrder::ACQUIRE, AtomicMemoryOrder::RELAXED>(
			expected, State::LOCKED_NOWAITERS
		);	// 1: desired value: locked, no other waiting
	}

	ALWAYS_INLINE bool trylock_timeout(const struct timespec* timeout_relative)
	{
		int expected_found = State::UNLOCKED;  // 0: expected value: unlocked
		bool result = true;
		// memory order: critical section below, therefore to an atomic-aquire here here (but only in case of success)
		if (value.compare_exchange_strong<AtomicMemoryOrder::ACQUIRE, AtomicMemoryOrder::RELAXED>(
				expected_found, State::LOCKED_NOWAITERS
			)
			== false) [[unlikely]]	// 1: desired value: locked, no other waiting
		{							// == false : is already locked, c contains found value
			result = wait_timeout(
				expected_found,
				timeout_relative
			);	// wait for futex, value is now 1 (LOCKED_NOWAITERS) or 2 (LOCKED_WAITERS)
		}
		return result;
	}

	ALWAYS_INLINE void unlock(void)	 // we are per definition the only running code inside the lock
	{								 // value before = 2 (LOCKED_WAITERS) or 1 (LOCKED_NOWAITERS)
		// memory order: critical section above, therefore to an atomic-release here
		if (value.fetch_sub<AtomicMemoryOrder::RELEASE>(1) == State::LOCKED_WAITERS)
			[[unlikely]]  // is someone waiting?
		{				  // U.Drepper uses a sub_fetch, we use a fetch_sub
			wake();
		}
	}

protected:
	void wait(int cached_state);
	bool wait_timeout(int cached_state, const struct timespec* timeout_relative);
	void wake(void);
};

}  // namespace ConcurrentFW


#endif	// CONCURRENTFW_FUTEX_HPP_
