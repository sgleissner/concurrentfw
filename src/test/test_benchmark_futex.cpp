/*
 * test_benchmark_futex.cpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#include <iostream>
#include <cstdint>
#include <pthread.h>
#include <time.h>
#include <chrono>
#include <thread>
#include <vector>
#include <tuple>
#include <functional>

#include <concurrentfw/futex.hpp>
#include <concurrentfw/atomic.hpp>


///////////////////////////////////////////////////////////////////////////////////////////
// mutex template
///////////////////////////////////////////////////////////////////////////////////////////

enum class MutexType : bool
{
	GLIBC = false,
	CONCURRENTFW = true
};

template<MutexType TYPE>
class TestMutex
{};

template<>
class TestMutex<MutexType::GLIBC>
{
public:
	ALWAYS_INLINE void lock()
	{
		pthread_mutex_lock(&mutex);
	}

	ALWAYS_INLINE bool trylock()
	{
		return (pthread_mutex_trylock(&mutex) == 0);
	}

	ALWAYS_INLINE bool trylock_timeout(const struct timespec& timeout_relative)
	{
		struct timespec abs_timeout;
		(void) clock_gettime(CLOCK_REALTIME, &abs_timeout);

		abs_timeout.tv_nsec += timeout_relative.tv_nsec;
		abs_timeout.tv_sec += timeout_relative.tv_sec;

		if (abs_timeout.tv_nsec >= 1000000000)
		{
			abs_timeout.tv_nsec -= 1000000000;
			abs_timeout.tv_sec += 1;
		}

		return (pthread_mutex_timedlock(&mutex, &abs_timeout) == 0);
	}

	ALWAYS_INLINE void unlock()
	{
		pthread_mutex_unlock(&mutex);
	}

private:
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
};

template<>
class TestMutex<MutexType::CONCURRENTFW>
{
public:
	ALWAYS_INLINE void lock()
	{
		futex.lock();
	}

	ALWAYS_INLINE bool trylock()
	{
		return futex.trylock();
	}

	ALWAYS_INLINE bool trylock_timeout(const struct timespec& timeout_relative)
	{
		return futex.trylock_timeout(&timeout_relative);
	}

	ALWAYS_INLINE void unlock()
	{
		futex.unlock();
	}

private:
	ConcurrentFW::Futex futex;
};

///////////////////////////////////////////////////////////////////////////////////////////
// mutex benchmark class
///////////////////////////////////////////////////////////////////////////////////////////


template<MutexType TYPE, typename PASSES>
class TestMutexBenchmark
{
	using ThreadTuple = std::tuple<
		std::thread,	  // thread
		TestMutex<TYPE>,  // mutex for counter
		PASSES,			  // counter success (locked)
		ConcurrentFW::Atomic<PASSES>>;

public:
	auto test_lock_unlock(PASSES passes)
	{
		TestMutex<TYPE> m;

		auto start = now();
		for (PASSES i = 0; i < passes; i++)
		{
			m.lock();
			m.unlock();
		}

		return diff(now(), start);
	}

	auto test_thread_lock_unlock(const uint32_t threads_no, const PASSES passes)
	{
		std::vector<ThreadTuple> thread_vector(threads_no);

		auto start = now();

		uint32_t number = 0;
		for (ThreadTuple& t : thread_vector)
			std::get<0>(t) = std::thread(
				[](uint32_t threads_no, std::vector<ThreadTuple>& thread_tuple, PASSES passes, uint32_t thread_no)
				{
					for (PASSES i = 0; i < passes; i++)
					{
						uint32_t thread_access = (i + thread_no) % threads_no;

						std::get<1>(thread_tuple[thread_access]).lock();

						volatile PASSES counter = std::get<2>(thread_tuple[thread_access]);
						counter = counter + 1;	// '++' for volatile is outdated
						std::get<2>(thread_tuple[thread_access]) = counter;

						std::get<1>(thread_tuple[thread_access]).unlock();
					}
				},
				threads_no,
				std::ref(thread_vector),
				passes,
				number++
			);

		for (ThreadTuple& t : thread_vector)
			std::get<0>(t).join();

		for (ThreadTuple& t : thread_vector)
			if (std::get<2>(t) != passes)
				std::cerr << "Error: Counter=" << std::get<2>(t) << " (must be " << passes << ")" << std::endl;
		//			else
		//				std::cout << "Counter=" << std::get<2>(t) << std::endl;

		return diff(now(), start);
	}

	auto test_thread_trylock_fail(const uint32_t threads_no, const PASSES passes)
	{
		std::vector<ThreadTuple> thread_vector(threads_no);

		auto start = now();

		for (ThreadTuple& t : thread_vector)
		{
			std::get<1>(t).lock();
			std::get<2>(t) = 0;
			std::get<3>(t).store(0);
		}

		uint32_t number = 0;
		for (ThreadTuple& t : thread_vector)
			std::get<0>(t) = std::thread(
				[](uint32_t threads_no, std::vector<ThreadTuple>& thread_tuple, PASSES passes, uint32_t thread_no)
				{
					for (PASSES i = 0; i < passes; i++)
					{
						uint32_t thread_access = (i + thread_no) % threads_no;

						if (std::get<1>(thread_tuple[thread_access]).trylock())
						{
							volatile PASSES counter = std::get<2>(thread_tuple[thread_access]);
							counter = counter + 1;	// '++' for volatile is outdated
							std::get<2>(thread_tuple[thread_access]) = counter;

							std::get<1>(thread_tuple[thread_access]).unlock();
						}
						else
						{
							std::get<3>(thread_tuple[thread_access]).fetch_add(1);
						}
					}
				},
				threads_no,
				std::ref(thread_vector),
				passes,
				number++
			);

		for (ThreadTuple& t : thread_vector)
			std::get<0>(t).join();

		for (ThreadTuple& t : thread_vector)
			std::get<1>(t).unlock();

		for (ThreadTuple& t : thread_vector)
			if ((std::get<2>(t) != 0) && (std::get<3>(t).load() != passes))
				std::cerr << "Error: Counter (locked)=" << std::get<2>(t) << " (must be 0), "
						  << "Counter (unlocked)=" << (std::get<3>(t).load()) << " (must be " << passes << ")"
						  << std::endl;
		//			else
		//				std::cout << "Counter (locked)=" << std::get<2>(t)
		//					<< ", Counter (unlocked)=" << (std::get<3>(t).load()) << std::endl;

		return diff(now(), start);
	}

	auto test_thread_trylock(const uint32_t threads_no, const PASSES passes)
	{
		std::vector<ThreadTuple> thread_vector(threads_no);

		auto start = now();

		for (ThreadTuple& t : thread_vector)
		{
			std::get<2>(t) = 0;
			std::get<3>(t).store(0);
		}

		uint32_t number = 0;
		for (ThreadTuple& t : thread_vector)
			std::get<0>(t) = std::thread(
				[](uint32_t threads_no, std::vector<ThreadTuple>& thread_tuple, PASSES passes, uint32_t thread_no)
				{
					for (PASSES i = 0; i < passes; i++)
					{
						uint32_t thread_access = (i + thread_no) % threads_no;

						if (std::get<1>(thread_tuple[thread_access]).trylock())
						{
							volatile PASSES counter = std::get<2>(thread_tuple[thread_access]);
							counter = counter + 1;	// '++' for volatile is outdated
							std::get<2>(thread_tuple[thread_access]) = counter;

							std::get<1>(thread_tuple[thread_access]).unlock();
						}
						else
						{
							std::get<3>(thread_tuple[thread_access]).fetch_add(1);
						}
					}
				},
				threads_no,
				std::ref(thread_vector),
				passes,
				number++
			);

		for (ThreadTuple& t : thread_vector)
			std::get<0>(t).join();

		for (ThreadTuple& t : thread_vector)
			if ((std::get<2>(t) + (std::get<3>(t).load())) != passes)
				std::cerr << "Error: Counter (locked)=" << std::get<2>(t)
						  << ", Counter (unlocked)=" << (std::get<3>(t).load()) << " (must be " << passes
						  << " together)" << std::endl;
		//			else
		//				std::cout << "Counter (locked)=" << std::get<2>(t)
		//					<< ", Counter (unlocked)=" << (std::get<3>(t).load()) << std::endl;

		return diff(now(), start);
	}


private:
	static std::chrono::time_point<std::chrono::steady_clock> now()
	{
		return std::chrono::steady_clock::now();
	}

	static auto diff(
		const std::chrono::time_point<std::chrono::steady_clock> clock_now,
		const std::chrono::time_point<std::chrono::steady_clock> clock_start
	)
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(clock_now - clock_start).count();
	}
};

///////////////////////////////////////////////////////////////////////////////////////////
// helper
///////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
constexpr T pot(const uint16_t p)
{
	T result = 1;
	for (uint16_t i = 0; i < p; i++)
		result *= 10;
	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////
// run benchmarks
///////////////////////////////////////////////////////////////////////////////////////////

void test_benchmark_futex()
{
	using Passes = uint32_t;
	constexpr uint16_t passes_pot = 7;	// 10^x passes
	constexpr Passes passes = pot<Passes>(passes_pot);

	TestMutexBenchmark<MutexType::GLIBC, Passes> benchmark_glibc;
	TestMutexBenchmark<MutexType::CONCURRENTFW, Passes> benchmark_concurrentfw;

	uint32_t time_ms_glibc;
	uint32_t time_ms_concurrentfw;

	uint32_t hw_threads = std::thread::hardware_concurrency();

	std::cout << "Benchmarking single thread 10^" << passes_pot << " * lock/unlock nonblocking mutex" << std::endl;
	std::cout << "glibc mutex: ";
	time_ms_glibc = benchmark_glibc.test_lock_unlock(passes);
	std::cout << " " << time_ms_glibc << " ms" << std::endl;
	std::cout << "concurrentfw futex: ";
	time_ms_concurrentfw = benchmark_concurrentfw.test_lock_unlock(passes);
	std::cout << " " << time_ms_concurrentfw << " ms (factor "
			  << static_cast<float>(time_ms_glibc) / static_cast<float>(time_ms_concurrentfw) << ")" << std::endl
			  << std::endl;

	std::cout << "Benchmarking " << hw_threads << " threads with 10^" << passes_pot << " * lock/unlock blocking mutexes"
			  << std::endl;
	std::cout << "glibc mutex: ";
	time_ms_glibc = benchmark_glibc.test_thread_lock_unlock(hw_threads, passes);
	std::cout << " " << time_ms_glibc << " ms" << std::endl;
	std::cout << "concurrentfw futex: ";
	time_ms_concurrentfw = benchmark_concurrentfw.test_thread_lock_unlock(hw_threads, passes);
	std::cout << " " << time_ms_concurrentfw << " ms (factor "
			  << static_cast<float>(time_ms_glibc) / static_cast<float>(time_ms_concurrentfw) << ")" << std::endl
			  << std::endl;

	std::cout << "Benchmarking " << hw_threads << " threads with 10^" << passes_pot
			  << " * trylock already blocked mutexes" << std::endl;
	std::cout << "glibc mutex: ";
	time_ms_glibc = benchmark_glibc.test_thread_trylock_fail(hw_threads, passes);
	std::cout << " " << time_ms_glibc << " ms" << std::endl;
	std::cout << "concurrentfw futex: ";
	time_ms_concurrentfw = benchmark_concurrentfw.test_thread_trylock_fail(hw_threads, passes);
	std::cout << " " << time_ms_concurrentfw << " ms (factor "
			  << static_cast<float>(time_ms_glibc) / static_cast<float>(time_ms_concurrentfw) << ")" << std::endl
			  << std::endl;

	std::cout << "Benchmarking " << hw_threads << " threads with 10^" << passes_pot << " * trylock mutexes"
			  << std::endl;
	std::cout << "glibc mutex: ";
	time_ms_glibc = benchmark_glibc.test_thread_trylock(hw_threads, passes);
	std::cout << " " << time_ms_glibc << " ms" << std::endl;
	std::cout << "concurrentfw futex: ";
	time_ms_concurrentfw = benchmark_concurrentfw.test_thread_trylock(hw_threads, passes);
	std::cout << " " << time_ms_concurrentfw << " ms (factor "
			  << static_cast<float>(time_ms_glibc) / static_cast<float>(time_ms_concurrentfw) << ")" << std::endl
			  << std::endl;

	// trylock_timeout

	// trylock_timeout_fail
}
