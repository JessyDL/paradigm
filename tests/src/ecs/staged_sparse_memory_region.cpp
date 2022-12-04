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
	auto t0										 = suite<"staged_sparse_memory_region_t", "ecs", "psl", "collections">(
	   litmus::generator::range<size_t, 0, 2> {}) = [](auto index) {
		 constexpr std::array<psl::ecs::entity, 3> sizes {50, 5000, 50000};
		 ssmr_t val = ssmr_t::instantiate<float>();

		 expect(val.size()) == 0;
		 expect(val.empty());

		 section<"reserve">() = [&]() {
			 val.reserve(sizes[index]);
			 expect(val.size()) == 0;
		 };

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
		 };
	};
}	 // namespace
