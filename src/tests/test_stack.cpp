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

#include <concurrentfw/atomic.hpp>
#include <concurrentfw/stack.hpp>

// global stacks must be accessible by threads
static ConcurrentFW::Stack<uint64_t>* stacks {nullptr};	 // vector<> won't work, as Stack<> is not move-constructable
static ConcurrentFW::Atomic<bool> end_test {false};


static void worker(const uint32_t thread, const uint32_t threads, uint64_t start, uint64_t length)
{
	while (length-- > 0)
		stacks[thread].emplace(start++);

	uint32_t push_thread = 0;
	uint8_t method = 0;
	while (!end_test.load<ConcurrentFW::RELAXED>())
	{
		switch (method)
		{
			case 0:
				stacks[thread].bulk_pop_visit(
					[&push_thread, &threads](uint64_t& element)
					{
						stacks[push_thread].push(element);
						push_thread = (push_thread + 1) % threads;
					}
				);
				break;
			case 1:
				stacks[thread].bulk_pop_reverse_visit(
					[&push_thread, &threads](uint64_t& element)
					{
						stacks[push_thread].push(element);
						push_thread = (push_thread + 1) % threads;
					}
				);
				break;
			default:  // case 2
				uint64_t value;
				uint64_t max = length;
				while (max-- && stacks[thread].pop(value))
				{
					stacks[push_thread].push(value);
					push_thread = (push_thread + 1) % threads;
				}
				break;
		}
		method = (method + 1) % 3;
	}
}

static std::pair<uint64_t, uint64_t> test_stack(
	uint32_t hw_threads, uint64_t elements_per_thread, std::chrono::seconds runtime
)
{
	std::vector<std::thread> threads;
	threads.reserve(hw_threads);
	stacks = new ConcurrentFW::Stack<uint64_t>[hw_threads];

	const uint64_t expected = elements_per_thread * hw_threads * (elements_per_thread * hw_threads - 1) / 2;

	for (uint32_t thread = 0; thread < hw_threads; thread++)
	{
		uint32_t start = elements_per_thread * thread;
		threads.emplace_back(worker, thread, hw_threads, start, elements_per_thread);
	}

	std::this_thread::sleep_for(runtime);
	end_test.store<ConcurrentFW::RELAXED>(true);

	for (auto& thread : threads)
		thread.join();

	uint64_t counted = 0;
	for (uint32_t stack_no = 0; stack_no < hw_threads; stack_no++)
	{
		uint64_t popped;
		while (stacks[stack_no].pop(popped))
			counted += popped;
	}

	delete[] stacks;
	threads.clear();

	return {expected, counted};
}

TEST_CASE("check of concurrent stack", "[stack]")
{
	uint32_t hw_threads = std::thread::hardware_concurrency();
	constexpr uint64_t elements_per_thread {1000};
	constexpr std::chrono::seconds runtime(1);


	INFO("available hardware threads: " << hw_threads);

	auto result = test_stack(hw_threads, elements_per_thread, runtime);

	CHECK(result.first == result.second);
}
