/*
 *   test_stack.cpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */

#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <thread>
#include <cstdint>
#include <utility>
#include <chrono>
#include <tuple>

#include <concurrentfw/atomic.hpp>
#include <concurrentfw/stack.hpp>


TEST_CASE("check of one stack", "[stack]")
{
	ConcurrentFW::Stack test_stack;
	uint8_t block[64];

	CHECK(test_stack.pop() == nullptr);
	CHECK_THROWS(test_stack.push(nullptr));
	CHECK_NOTHROW(test_stack.push(&block[0]));
	void* retval;
	CHECK((retval = test_stack.pop()) == &block[0]);
	CHECK(test_stack.pop() == nullptr);
}

static ConcurrentFW::Stack* stacks {nullptr};  // vector<> won't work, as Stack<> is not move-constructable
static ConcurrentFW::Atomic<bool> end_test {false};
static ConcurrentFW::Atomic<uint64_t> overall_stack_operations {0};

static void worker(const uint32_t thread, const uint32_t threads)
{
	uint64_t stack_operations = 0;
	uint32_t push_thread = 0;

	while (!end_test.load<ConcurrentFW::RELAXED>())
	{
		void* block;
		while (!end_test.load<ConcurrentFW::RELAXED>() && ((block = stacks[thread].pop()) != nullptr))
		{
			stack_operations += 2;
			stacks[push_thread].push(block);
			push_thread = (push_thread + 1) % threads;
		}
		stack_operations++;
	}

	stack_operations--;

	overall_stack_operations.add_fetch<ConcurrentFW::RELAXED>(stack_operations);
}

static std::tuple<uint64_t /* expected result */, uint64_t /* result */, uint64_t /* performance */> test_stack(
	uint32_t hw_threads, uint64_t elements_per_thread, std::chrono::seconds runtime
)
{
	std::vector<std::thread> threads;
	threads.reserve(hw_threads);
	stacks = new ConcurrentFW::Stack[hw_threads];

	uint64_t expected = 0;
	for (uint32_t stack_no = 0; stack_no < hw_threads; stack_no++)
	{
		for (uint64_t i = 0; i < elements_per_thread; i++)
		{
			stacks[stack_no].push(malloc(64));
			expected++;
		}
	}


	for (uint32_t thread = 0; thread < hw_threads; thread++)
	{
		threads.emplace_back(worker, thread, hw_threads);
	}

	std::this_thread::sleep_for(runtime);
	end_test.store<ConcurrentFW::RELAXED>(true);

	for (auto& thread : threads)
		thread.join();

	uint64_t counted = 0;
	for (uint32_t stack_no = 0; stack_no < hw_threads; stack_no++)
	{
		void* popped;
		while ((popped = stacks[stack_no].pop()))
		{
			free(popped);
			counted++;
		}
	}

	delete[] stacks;
	threads.clear();

	return {expected, counted, overall_stack_operations.load<ConcurrentFW::RELAXED>()};
}

TEST_CASE("check of concurrent stacks", "[stack]")
{
	uint32_t hw_threads = std::thread::hardware_concurrency();
	constexpr uint64_t elements_per_thread {1000};
	constexpr std::chrono::seconds runtime(1);

	INFO("available hardware threads: " << hw_threads);
	auto result = test_stack(hw_threads, elements_per_thread, runtime);
	INFO("stack operations (push+pop): " << std::get<2>(result) << " (also new/delete done)");
	CHECK(std::get<0>(result) == std::get<1>(result));
	//	CHECK(false);
}
