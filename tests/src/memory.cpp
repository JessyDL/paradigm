#include <numeric>

#include "memory.h"
#include "psl/memory/region.hpp"

size_t free_size(const memory::region& region)
{
	auto available = region.allocator()->available();
	if(available.size() == 0) return 0;
	return std::accumulate(std::begin(available),
						   std::end(available),
						   size_t(0),
						   [](size_t sum, const memory::range_t& element) { return sum + element.size(); });
}

size_t used_size(const memory::region& region)
{
	auto committed = region.allocator()->committed();
	if(committed.size() == 0) return 0;
	return std::accumulate(std::begin(committed),
						   std::end(committed),
						   size_t(0),
						   [](size_t sum, const memory::range_t& element) { return sum + element.size(); });
}


size_t used_size(const std::vector<std::optional<memory::segment>>& segments)
{
	return std::accumulate(std::begin(segments),
						   std::end(segments),
						   size_t(0u),
						   [](size_t sum, const std::optional<memory::segment>& element) {
							   return sum + ((element.has_value()) ? element.value().range().size() : 0);
						   });
}

std::vector<std::vector<size_t>> size_set {{2, 8, 4, 32}, {256, 192, 128, 384}, {384, 4, 8, 32, 128, 192, 64, 288}};

#define DETAILED_TESTS

#include <litmus/expect.hpp>
#include <litmus/section.hpp>
#include <litmus/suite.hpp>
#include <litmus/generator/range.hpp>

namespace
{
	auto m = litmus::suite<"memory">(litmus::generator::range<size_t, 0,2>{}) = [](auto index) {
		using namespace litmus;
		const size_t alignment		 = 1;
		const size_t region_size	 = 1024 * 1024;
		const bool physically_backed = true;
		memory::default_allocator* allocator {new memory::default_allocator {physically_backed}};
		memory::region region {region_size, alignment, allocator};

		require(region_size) <= region.size();
		require((std::uintptr_t)allocator) == (std::uintptr_t)region.allocator();
		require(physically_backed) == region.allocator()->is_physically_backed();
		require(alignment) == region.alignment();

		const auto& set = size_set[index];

		section<"allocations/deallocations - sequential">() = [&] {
			std::vector<std::optional<memory::segment>> segments;
			size_t expected_size = 0u;
			for(auto i = 0; i < 1000; ++i)
			{
				const size_t size = set[std::rand() % set.size()];
				if(size + expected_size > region.size()) break;
				segments.emplace_back(region.allocate(size));
				expected_size += size;

				require(segments[segments.size() - 1].has_value());
			}
			require(free_size(region)) == (region.size() - expected_size);
			require(used_size(region)) == expected_size;

			for(auto i = 0; i < 500; ++i)
			{
				auto index = std::rand() % segments.size();
				auto segm  = segments[index];
				expected_size -= segm.value().range().size();

				require(region.deallocate(segm));
				segments.erase(std::next(std::begin(segments), index));
				const auto available = region.allocator()->available();
				;
				for(const auto& av : available)
				{
					require(av.size()) > 0;
				}
			}
		};


		section<"allocations/deallocations - interleaved">() = [&] {
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
						require(segments[segments.size() - 1].has_value());
#ifdef DETAILED_TESTS
						const auto free = free_size(region);
						require(free) == (region.size() - expected_size);
						const auto calculated_usage = used_size(segments);
						require(calculated_usage) == expected_size;
						const auto used = used_size(region);
						require(used) == expected_size;
#endif
					}
					else if(segments.size() > 0)
					{
						auto index = std::rand() % segments.size();
						auto segm  = segments[index];
						expected_size -= segm.value().range().size();

						require(region.deallocate(segm));
						segments.erase(std::next(std::begin(segments), index));
#ifdef DETAILED_TESTS
						const auto free = free_size(region);
						require(free) == (region.size() - expected_size);
						const auto available = region.allocator()->available();
						;
						for(const auto& av : available)
						{
							require(av.size()) > 0;
						}
						const auto calculated_usage = used_size(segments);
						require(calculated_usage) == expected_size;
						const auto used = used_size(region);
						require(used) == expected_size;
#endif
					}
					else
					{
						++no_ops;
					}
				}
				{
					const auto free				= free_size(region);
					const auto used				= used_size(region);
					const auto calculated_usage = used_size(segments);
					require(calculated_usage) == expected_size;
					require(free) == (region.size() - expected_size);
					require(used) == expected_size;
					require(free + used) == region.size();
					require(no_ops) == 0;
				}
			}
		};
	};
}	 // namespace
