#include "psl/ecs/details/staged_sparse_memory_region.hpp"
#include <array>
using ssmr_t = psl::ecs::details::staged_sparse_memory_region_t;

#include <litmus/expect.hpp>
#include <litmus/generator/range.hpp>
#include <litmus/section.hpp>
#include <litmus/suite.hpp>

using namespace litmus;

namespace
{
	void compare_ranges(ssmr_t& val)
	{
		require(val.indices(ssmr_t::stage_range_t::ALL).size()) == val.dense<float>(ssmr_t::stage_range_t::ALL).size();
		require(val.indices(ssmr_t::stage_range_t::ADDED).size()) ==
		  val.dense<float>(ssmr_t::stage_range_t::ADDED).size();
		require(val.indices(ssmr_t::stage_range_t::SETTLED).size()) ==
		  val.dense<float>(ssmr_t::stage_range_t::SETTLED).size();
		require(val.indices(ssmr_t::stage_range_t::REMOVED).size()) ==
		  val.dense<float>(ssmr_t::stage_range_t::REMOVED).size();
		require(val.indices(ssmr_t::stage_range_t::ALIVE).size()) ==
		  val.dense<float>(ssmr_t::stage_range_t::ALIVE).size();
		require(val.indices(ssmr_t::stage_range_t::TERMINAL).size()) ==
		  val.dense<float>(ssmr_t::stage_range_t::TERMINAL).size();
	}

	void insert_elements(ssmr_t& val, psl::ecs::entity value_count)
	{
		auto start_count_added	 = val.indices(ssmr_t::stage_range_t::ADDED).size();
		auto start_count_settled = val.indices(ssmr_t::stage_range_t::SETTLED).size();
		float default_value		 = 10.0f;

		section<"with data">() = [&]() {
			for(psl::ecs::entity i = 0; i < value_count; ++i, ++default_value)
			{
				val.insert(i, default_value);
			}
		};
		section<"without data (post modification)">() = [&]() {
			for(psl::ecs::entity i = 0; i < value_count; ++i)
			{
				val.insert(i);
			}
			for(psl::ecs::entity i = 0; i < value_count; ++i, ++default_value)
			{
				require(val.set(i, default_value));
			}
		};

		default_value -= value_count;
		for(psl::ecs::entity i = 0; i < value_count; ++i, ++default_value)
		{
			require(val.at<float>(i, ssmr_t::stage_range_t::ADDED)) == default_value;
		}
		require(val.indices(ssmr_t::stage_range_t::ADDED).size()) == value_count + start_count_added;
		require(val.indices(ssmr_t::stage_range_t::SETTLED).size()) == start_count_settled;
		require(val.indices().size()) == start_count_settled;

		compare_ranges(val);
	}

	void remove_elements(ssmr_t& val, psl::ecs::entity first, psl::ecs::entity last)
	{
		auto start_count_all	 = val.indices(ssmr_t::stage_range_t::ALL).size();
		auto start_count_alive	 = val.indices(ssmr_t::stage_range_t::ALIVE).size();
		auto start_count_removed = val.indices(ssmr_t::stage_range_t::REMOVED).size();

		size_t erased {0};
		section<"ranged">() = [&]() { erased = val.erase(first, last); };
		section<"manual">() = [&]() {
			for(auto i = first; i != last; ++i) erased += val.erase(i);
		};

		for(auto i = first; i < last; ++i)
		{
			require(!val.has(i));
		}
		require(val.indices(ssmr_t::stage_range_t::ALL).size()) == start_count_all;
		require(val.indices(ssmr_t::stage_range_t::ALIVE).size()) == start_count_alive - erased;
		require(val.indices(ssmr_t::stage_range_t::REMOVED).size()) == start_count_removed + erased;

		compare_ranges(val);
	}

	auto t0										 = suite<"staged_sparse_memory_region_t", "ecs", "psl", "collections">(
	   litmus::generator::range<size_t, 0, 2> {}) = [](auto index) {
		 constexpr std::array<psl::ecs::entity, 3> sizes {50, 5000, 50000};
		 constexpr std::array<std::pair<psl::ecs::entity, psl::ecs::entity>, 3> erase_ranges {
											   {{10, 35}, {4500, 5500}, {10000, 50000}}};
		 ssmr_t val = ssmr_t::instantiate<float>();
		 require(val.indices().size()) >= 0;
		 require(val.dense<float>().size()) >= 0;

		 section<"inserting several elements">(val, sizes[index])								  = insert_elements;
		 section<"removing elements">(val, erase_ranges[index].first, erase_ranges[index].second) = remove_elements;
	};
}	 // namespace
