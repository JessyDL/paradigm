#include "stdafx_tests.h"
#include "memory.h"
#include "memory/region.h"

size_t free_size(const memory::region& region)
{
	auto available = region.allocator()->available();
	if(available.size() == 0) return 0;
	return std::accumulate(std::begin(available), std::end(available), size_t(0),
						   [](size_t sum, const memory::range& element) { return sum + element.size(); });
}

size_t used_size(const memory::region& region)
{
	auto committed = region.allocator()->committed();
	if(committed.size() == 0) return 0;
	return std::accumulate(std::begin(committed), std::end(committed), size_t(0),
						   [](size_t sum, const memory::range& element) { return sum + element.size(); });
}


size_t used_size(const std::vector<std::optional<memory::segment>>& segments)
{
	return std::accumulate(std::begin(segments), std::end(segments), size_t(0u),
						   [](size_t sum, const std::optional<memory::segment>& element)
						   {
							   return sum + ((element.has_value()) ? element.value().range().size() : 0);
						   });
}

std::vector<std::vector<size_t>> size_set{{2, 8, 4, 32}, {256, 192, 128, 384}, {384, 4, 8, 32, 128, 192, 64, 288}};

#define DETAILED_TESTS

TEST_CASE("memory", "[memory]")
{
	const size_t alignment		 = 1;
	const size_t region_size	 = 1024 * 1024;
	const bool physically_backed = true;
	memory::default_allocator* allocator{new memory::default_allocator{physically_backed}};
	memory::region region{region_size, alignment, allocator};

	REQUIRE(region_size <= region.size());
	REQUIRE(allocator == region.allocator());
	REQUIRE(physically_backed == region.allocator()->is_physically_backed());
	REQUIRE(alignment == region.alignment());

	auto index		= GENERATE(0, 1, 2);
	const auto& set = size_set[index];
	SECTION("allocations/deallocations - sequential")
	{
		std::vector<std::optional<memory::segment>> segments;
		size_t expected_size = 0u;
		for(auto i = 0; i < 1000; ++i)
		{
			const size_t size = set[std::rand() % set.size()];
			if(size + expected_size > region.size()) break;
			segments.emplace_back(region.allocate(size));
			expected_size += size;

			REQUIRE(segments[segments.size() - 1].has_value());
		}
		REQUIRE(free_size(region) == (region.size() - expected_size));
		REQUIRE(used_size(region) == expected_size);

		for(auto i = 0; i < 500; ++i)
		{
			auto index = std::rand() % segments.size();
			auto segm  = segments[index];
			expected_size -= segm.value().range().size();

			REQUIRE(region.deallocate(segm));
			segments.erase(std::next(std::begin(segments), index));
			const auto available = region.allocator()->available();;
			for(const auto& av : available)
			{
				REQUIRE(av.size() > 0);
			}
		}
	}

	SECTION("allocations/deallocations - interleaved")
	{
		std::vector<std::optional<memory::segment>> segments;
		size_t expected_size = 0u;

		for(size_t pass = 0; pass < 20; ++pass)
		{
			const size_t maxCount = std::rand() % 512 + 256;
			size_t no_ops		  = 0;
			for(size_t i = 0; i < maxCount; ++i)
			{
				const size_t size = set[std::rand() % set.size()];
				if(segments.size() < 30 || (std::rand() % 2 == 0 && size + expected_size > region.size()))
				{
					segments.emplace_back(region.allocate(size));
					expected_size += size;
					REQUIRE(segments[segments.size() - 1].has_value());
				#ifdef DETAILED_TESTS
					const auto free = free_size(region);
					REQUIRE(free == (region.size() - expected_size));
					const auto calculated_usage = used_size(segments);
					REQUIRE(calculated_usage == expected_size);
					const auto used = used_size(region);
					REQUIRE(used == expected_size);
				#endif
				}
				else if(segments.size() > 0)
				{
					auto index = std::rand() % segments.size();
					auto segm  = segments[index];
					expected_size -= segm.value().range().size();

					REQUIRE(region.deallocate(segm));
					segments.erase(std::next(std::begin(segments), index));
				#ifdef DETAILED_TESTS
					const auto free = free_size(region);
					REQUIRE(free == (region.size() - expected_size));
					const auto available = region.allocator()->available();;
					for(const auto& av : available)
					{
						REQUIRE(av.size() > 0);
					}
					const auto calculated_usage = used_size(segments);
					REQUIRE(calculated_usage == expected_size);
					const auto used = used_size(region);
					REQUIRE(used == expected_size);
				#endif
				}
				else
				{
					++no_ops;
				}
			}
			{
				const auto free = free_size(region);
				const auto used = used_size(region);
				const auto calculated_usage = used_size(segments);
				REQUIRE(calculated_usage == expected_size);
				REQUIRE(free == (region.size() - expected_size));
				REQUIRE(used == expected_size);
				REQUIRE(free + used == region.size());
				REQUIRE(no_ops == 0);
			}
		}
	}
}