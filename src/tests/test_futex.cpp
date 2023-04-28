/*
 * test_benchmark_futex.cpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#include <catch2/catch_test_macros.hpp>

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
	struct alignas(64) ThreadTuple	// std::tuple<> may not be aligned to cache lines
	{
		alignas(64) std::thread thread;							 // thread
		alignas(64) TestMutex<TYPE> mutex;						 // mutex for counter
		alignas(64) PASSES passes_locked;						 // counter (protected by mutex)
		alignas(64) ConcurrentFW::Atomic<PASSES> passes_atomic;	 // counter (but atomic)
	};

	using WorkerThread = std::function<void(size_t, size_t, std::vector<ThreadTuple>&, ConcurrentFW::Atomic<bool>&)>;

	using DurationSingle = std::chrono::duration<uint64_t, std::pico>;

	enum class CountResult : bool
	{
		CountLockedAndCompare = false,
		CountAtomic = true
	};

public:
	std::pair<DurationSingle, std::string> run_for(
		std::chrono::milliseconds runtime,
		std::vector<ThreadTuple>& workers,
		std::string_view benchmark_name,
		CountResult count_result,
		WorkerThread worker_thread
	)
	{
		ConcurrentFW::Atomic<bool> stop_threads {false};

		size_t threads_no = workers.size();
		size_t thread_no = 0;
		for (ThreadTuple& thread_tuple : workers)
			thread_tuple.thread
				= std::thread(worker_thread, threads_no, thread_no++, std::ref(workers), std::ref(stop_threads));

		std::this_thread::sleep_for(runtime);
		stop_threads.store<ConcurrentFW::RELAXED>(true);

		for (ThreadTuple& thread_tuple : workers)
			thread_tuple.thread.join();

		uint64_t passes {0};
		for (ThreadTuple& thread_tuple : workers)
			if (count_result == CountResult::CountLockedAndCompare)
			{
				if (thread_tuple.passes_locked != thread_tuple.passes_atomic.template load<ConcurrentFW::RELAXED>())
					throw std::logic_error("locked and atomic counters are different");
				passes += thread_tuple.passes_locked;
			}
			else
				passes += thread_tuple.passes_atomic.template load<ConcurrentFW::RELAXED>();

		DurationSingle duration_single = duration_cast<DurationSingle>(runtime) * threads_no / passes;

		std::stringstream info;
		info << "Benchmark: " << benchmark_name << ", single duration: " << duration_single
			 << ", threads: " << threads_no << ", passes: " << passes << ", runtime: " << runtime;

		return {duration_single, std::move(info).str()};
	}

	auto test_independent_lock_unlock(
		std::string_view benchmark_name, const size_t threads_no, std::chrono::milliseconds runtime
	)
	{
		std::vector<ThreadTuple> workers(threads_no);

		for (ThreadTuple& thread_tuple : workers)
		{
			thread_tuple.passes_locked = 0;
			thread_tuple.passes_atomic.template store<ConcurrentFW::RELAXED>(0);
		}

		auto result = run_for(
			runtime,
			workers,
			benchmark_name,
			CountResult::CountLockedAndCompare,
			[](size_t,	// unused, as threads are independent
			   size_t thread_no,
			   std::vector<ThreadTuple>& thread_tuple,
			   ConcurrentFW::Atomic<bool>& stop_thread)
			{
				while (stop_thread.load<ConcurrentFW::RELAXED>() == false)
				{
					thread_tuple[thread_no].mutex.lock();
					thread_tuple[thread_no].passes_locked += 1;
					thread_tuple[thread_no].mutex.unlock();
					thread_tuple[thread_no].passes_atomic.fetch_add(1);
				}
			}
		);

		return result;
	}

	auto test_dependent_lock_unlock(
		std::string_view benchmark_name, const size_t threads_no, std::chrono::milliseconds runtime
	)
	{
		std::vector<ThreadTuple> workers(threads_no);

		for (ThreadTuple& thread_tuple : workers)
		{
			thread_tuple.passes_locked = 0;
			thread_tuple.passes_atomic.template store<ConcurrentFW::RELAXED>(0);
		}

		auto result = run_for(
			runtime,
			workers,
			benchmark_name,
			CountResult::CountLockedAndCompare,
			[](size_t threads_no,
			   size_t,	// not used here
			   std::vector<ThreadTuple>& thread_tuple,
			   ConcurrentFW::Atomic<bool>& stop_thread)
			{
				size_t thread_access = 0;
				while (stop_thread.load<ConcurrentFW::RELAXED>() == false)
				{
					thread_tuple[thread_access].mutex.lock();
					thread_tuple[thread_access].passes_locked += 1;
					thread_tuple[thread_access].mutex.unlock();
					thread_tuple[thread_access].passes_atomic.fetch_add(1);
					thread_access = (thread_access + 1) % threads_no;
				}
			}
		);

		return result;
	}

	auto test_trylock_fail(std::string_view benchmark_name, const size_t threads_no, std::chrono::milliseconds runtime)
	{
		std::vector<ThreadTuple> workers(threads_no);

		for (ThreadTuple& thread_tuple : workers)
		{
			thread_tuple.mutex.lock();
			thread_tuple.passes_locked = 0;
			thread_tuple.passes_atomic.template store<ConcurrentFW::RELAXED>(0);
		}

		auto result = run_for(
			runtime,
			workers,
			benchmark_name,
			CountResult::CountAtomic,
			[](size_t threads_no,
			   size_t,	// not used here
			   std::vector<ThreadTuple>& thread_tuple,
			   ConcurrentFW::Atomic<bool>& stop_thread)
			{
				size_t thread_access = 0;
				while (stop_thread.load<ConcurrentFW::RELAXED>() == false)
				{
					if (thread_tuple[thread_access].mutex.trylock())
					{
						thread_tuple[thread_access].passes_locked += 1;
						thread_tuple[thread_access].mutex.unlock();
					}
					else
					{
						thread_tuple[thread_access].passes_atomic.fetch_add(1);
					}
					thread_access = (thread_access + 1) % threads_no;
				}
			}
		);

		for (ThreadTuple& thread_tuple : workers)
			thread_tuple.mutex.unlock();

		for (ThreadTuple& thread_tuple : workers)
			if (thread_tuple.passes_locked != 0)
			{
				std::stringstream error;
				error << "Error: Counter (locked)=" << thread_tuple.passes_locked << " (must be 0), "
					  << "Counter (atomic)=" << thread_tuple.passes_atomic.load();
				throw std::logic_error(error.str());
			}

		return result;
	}

	auto test_trylock(std::string_view benchmark_name, const size_t threads_no, std::chrono::milliseconds runtime)
	{
		std::vector<ThreadTuple> workers(threads_no);

		for (ThreadTuple& thread_tuple : workers)
		{
			thread_tuple.passes_locked = 0;
			thread_tuple.passes_atomic.template store<ConcurrentFW::RELAXED>(0);
		}

		auto result = run_for(
			runtime,
			workers,
			benchmark_name,
			CountResult::CountAtomic,
			[](size_t threads_no,
			   size_t,	// not used here
			   std::vector<ThreadTuple>& thread_tuple,
			   ConcurrentFW::Atomic<bool>& stop_thread)
			{
				size_t thread_access = 0;
				while (stop_thread.load<ConcurrentFW::RELAXED>() == false)
				{
					if (thread_tuple[thread_access].mutex.trylock())
					{
						thread_tuple[thread_access].passes_locked += 1;
						thread_tuple[thread_access].mutex.unlock();
					}
					else
					{
						thread_tuple[thread_access].passes_atomic.fetch_add(1);
					}
					thread_access = (thread_access + 1) % threads_no;
				}
			}
		);

		return result;
	}
};


///////////////////////////////////////////////////////////////////////////////////////////
// run benchmarks
///////////////////////////////////////////////////////////////////////////////////////////

double factor(auto glibc_duration, auto concurrentfw_duration)
{
	return static_cast<double>(glibc_duration.count()) / static_cast<double>(concurrentfw_duration.count());
}

static const uint32_t hw_threads = std::thread::hardware_concurrency();

using namespace std::chrono_literals;
using DurationRuntime = std::chrono::milliseconds;

using Passes = uint32_t;
//using Passes = uint64_t;

static constexpr DurationRuntime runtime = 1000ms;
static constexpr double min_speedup = 0.5;	// min. 50% speed compared to glib futex to pass tests

static TestMutexBenchmark<MutexType::GLIBC, Passes> benchmark_glibc;
static TestMutexBenchmark<MutexType::CONCURRENTFW, Passes> benchmark_concurrentfw;

TEST_CASE("check Independent Single Futex", "[futex]")
{
	auto [duration_glibc_independent_single, info_glibc_independent_single]
		= benchmark_glibc.test_independent_lock_unlock("Independent Single GLIBC", 1, runtime);
	auto [duration_concurrentfw_independent_single, info_concurrentfw_independent_single]
		= benchmark_concurrentfw.test_independent_lock_unlock("Independent Single ConcurrentFW", 1, runtime);
	double speedup_factor = factor(duration_glibc_independent_single, duration_concurrentfw_independent_single);
	INFO(info_glibc_independent_single);
	INFO(info_concurrentfw_independent_single);
	INFO("Factor: " << speedup_factor);
	CHECK(speedup_factor >= min_speedup);
}

TEST_CASE("check Independent Multi Futex", "[futex]")
{
	auto [duration_glibc_independent_multi, info_glibc_independent_multi]
		= benchmark_glibc.test_independent_lock_unlock("Independent Multi GLIBC", hw_threads, runtime);
	auto [duration_concurrentfw_independent_multi, info_concurrentfw_independent_multi]
		= benchmark_concurrentfw.test_independent_lock_unlock("Independent Multi ConcurrentFW", hw_threads, runtime);
	double speedup_factor = factor(duration_glibc_independent_multi, duration_concurrentfw_independent_multi);
	INFO(info_glibc_independent_multi);
	INFO(info_concurrentfw_independent_multi);
	INFO("Factor: " << speedup_factor);
	CHECK(speedup_factor >= min_speedup);
}

TEST_CASE("check Dependent Futex", "[futex]")
{
	auto [duration_glibc_dependent, info_glibc_dependent]
		= benchmark_glibc.test_dependent_lock_unlock("Dependent Multi GLIBC", hw_threads, runtime);
	auto [duration_concurrentfw_dependent, info_concurrentfw_dependent]
		= benchmark_concurrentfw.test_dependent_lock_unlock("Dependent Multi ConcurrentFW", hw_threads, runtime);
	double speedup_factor = factor(duration_glibc_dependent, duration_concurrentfw_dependent);
	INFO(info_glibc_dependent);
	INFO(info_concurrentfw_dependent);
	INFO("Factor: " << speedup_factor);
	CHECK(speedup_factor >= min_speedup);
}

TEST_CASE("check Trylock Fail Futex", "[futex]")
{
	auto [duration_glibc_trylock_fail, info_glibc_trylock_fail]
		= benchmark_glibc.test_trylock_fail("Trylock Fail GLIBC", hw_threads, runtime);
	auto [duration_concurrentfw_trylock_fail, info_concurrentfw_trylock_fail]
		= benchmark_concurrentfw.test_trylock_fail("Trylock Fail ConcurrentFW", hw_threads, runtime);
	double speedup_factor = factor(duration_glibc_trylock_fail, duration_concurrentfw_trylock_fail);
	INFO(info_glibc_trylock_fail);
	INFO(info_concurrentfw_trylock_fail);
	INFO("Factor: " << speedup_factor);
	CHECK(speedup_factor >= min_speedup);
}

TEST_CASE("check Trylock Futex", "[futex]")
{
	auto [duration_glibc_trylock, info_glibc_trylock]
		= benchmark_glibc.test_trylock("Trylock GLIBC", hw_threads, runtime);
	auto [duration_concurrentfw_trylock, info_concurrentfw_trylock]
		= benchmark_concurrentfw.test_trylock("Trylock ConcurrentFW", hw_threads, runtime);
	double speedup_factor = factor(duration_glibc_trylock, duration_concurrentfw_trylock);
	INFO(info_glibc_trylock);
	INFO(info_concurrentfw_trylock);
	INFO("Factor: " << speedup_factor);
	CHECK(speedup_factor >= min_speedup);
}
