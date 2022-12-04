#include "psl/ecs/details/staged_sparse_memory_region.hpp"
#include <array>
using ssmr_t = psl::ecs::details::staged_sparse_memory_region_t;
#define LITMUS_BREAK_ON_FAIL

#include <litmus/expect.hpp>
#include <litmus/generator/range.hpp>
#include <litmus/section.hpp>
#include <litmus/suite.hpp>

using namespace litmus;

namespace
{
	auto t0										 = suite<"staged_sparse_memory_region_t", "ecs", "psl", "collections">(
	   litmus::generator::range<size_t, 0, 2> {}) = [](auto index) {
		 constexpr std::array<psl::ecs::entity, 3> sizes {50, 5000, 50000};
		 ssmr_t val = ssmr_t::instantiate<float>();
		 require(val.indices().size()) >= 0;
		 require(val.dense<float>().size()) >= 0;

		 section<"inserting several elements">() = [&]() {
			 float default_value				= 10.0f;
			 const psl::ecs::entity value_count = sizes[index];

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
			 require(val.indices(ssmr_t::stage_range_t::ADDED).size()) == value_count;
			 require(val.indices(ssmr_t::stage_range_t::SETTLED).size()) == 0;
			 require(val.indices().size()) == 0;
			 require(val.dense<float>(ssmr_t::stage_range_t::ADDED).size()) == value_count;
			 require(val.dense<float>(ssmr_t::stage_range_t::SETTLED).size()) == 0;
			 require(val.dense<float>().size()) == 0;
		 };
	};
}	 // namespace
