#include "stdafx_tests.h"
#include "task_test.h"
#include "task.h"
#include <chrono>

namespace async = psl::async2;

uint64_t findSieveSize(uint64_t n)
{
	// For small n, the formula returns a value too low, so we can just
	// hardcode the sieve size to 5 (5th prime is 11).
	if(n < 6)
		return 13;

	// We can't find a prime that will exceed ~0UL.
	if(n >= (~0UL / log(~0UL)))
		return 0;

	// Binary search for the right value.
	uint64_t low = n;
	uint64_t high = ~0UL - 1;
	do
	{
		uint64_t mid = low + (high - low) / 2;
		double        guess = mid / log(mid);

		if(guess > n)
			high = (uint64_t)mid - 1;
		else
			low = (uint64_t)mid + 1;
	} while(low < high);
	return high + 1;
}

uint64_t find_nth_prime(uint64_t n)
{
	if(!n) return 1;           // "0th prime"
	if(!--n) return 2;         // first prime

	uint64_t sieveSize = findSieveSize(n);
	uint64_t count = 0;
	uint64_t max_i = sqrt(sieveSize - 1) + 1;

	if(sieveSize == 0)
		return 0;

	std::vector<bool> sieve(sieveSize);
	for(uint64_t i = 3; true; i += 2)
	{
		if(!sieve[i])
		{
			if(++count == n)
				return i;
			if(i >= max_i)
				continue;
			uint64_t j = i * i;
			uint64_t inc = i + i;
			uint64_t maxj = sieveSize - inc;
			// This loop checks j before adding inc so that we can stop
			// before j overflows.
			do
			{
				sieve[j] = true;
				if(j >= maxj)
					break;
				j += inc;
			} while(1);
		}
	}
	return 0;
}


TEST_CASE("free-floating tasks", "[async]")
{
	async::scheduler scheduler;
	size_t iteration_count = 64;

	std::vector<std::future<int>> results;
	for(size_t i = 0; i < iteration_count; ++i)
	{
		results.emplace_back(scheduler.schedule([]()
				  {
					  std::this_thread::sleep_for(std::chrono::microseconds{10});
					  return 5;
				  }).second);
	}


	auto future = scheduler.execute();

	REQUIRE(std::accumulate(std::begin(results), std::end(results), 0, [](int sum, std::future<int>& value) { return sum + value.get();}) == iteration_count * 5);
}

TEST_CASE("tasks with inter-dependencies", "[async]")
{
	async::scheduler scheduler;
	size_t iteration_count = 12 * 10;

	std::vector<uint64_t> shared_values{50045, 150020, 121005, 233100, 250045, 367000, 50045, 150020, 121005, 233100, 250045, 367000};
	std::vector<uint64_t> shared_output{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	uint64_t calculated_value = 0;
	for(auto val : shared_values)
		calculated_value += find_nth_prime(val);
	std::vector<std::future<uint64_t>> results;
	for(size_t i = 0; i < iteration_count; ++i)
	{
		auto pair{scheduler.schedule([i, &shared_values, &shared_output]()
												{
													++shared_output[i % shared_output.size()];
													return find_nth_prime(shared_values[i % shared_values.size()]);
									 })};
		
		async::barrier read_barrier{(std::uintptr_t)shared_values.data() + (i % shared_output.size()) * sizeof(uint64_t),
			(std::uintptr_t)shared_values.data() + ((i % shared_output.size()) + 1) * sizeof(uint64_t)};
		async::barrier write_barrier{(std::uintptr_t)shared_output.data() + (i % shared_values.size()) * sizeof(uint64_t),
			(std::uintptr_t)shared_output.data() + ((i % shared_values.size()) + 1) * sizeof(uint64_t),
			async::barrier_type::READ_WRITE};

		scheduler.dependency(pair.first, {read_barrier, write_barrier});
		results.emplace_back(std::move(pair.second));
	}

	auto future = scheduler.execute(async::launch::async);

	REQUIRE(std::accumulate(std::begin(results), std::end(results), uint64_t{0}, [](uint64_t sum, std::future<uint64_t>& value) { return sum + value.get(); }) == (iteration_count / shared_output.size()) * calculated_value);
}