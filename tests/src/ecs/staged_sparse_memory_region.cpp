#include "psl/ecs/details/staged_sparse_memory_region.hpp"
#include <array>
using ssmr_t = psl::ecs::details::staged_sparse_memory_region_t;

#include <litmus/expect.hpp>
#include <litmus/generator/range.hpp>
#include <litmus/section.hpp>
#include <litmus/suite.hpp>

using namespace litmus;

template <typename T, auto... Values>
using array_typed = litmus::generator::array_typed<T, Values...>;

using entity = psl::ecs::entity;

template <typename Fn>
auto any_of_n(auto first, auto last, Fn&& fn)
{
	for(auto i = first; i != last; ++i)
	{
		if(fn(i)) return true;
	}
	return false;
}

template <typename Fn>
auto none_of_n(auto first, auto last, Fn&& fn)
{
	for(auto i = first; i != last; ++i)
	{
		if(fn(i)) return false;
	}
	return true;
}

template <typename Fn>
auto all_of_n(auto first, auto last, Fn&& fn)
{
	for(auto i = first; i != last; ++i)
	{
		if(!fn(i)) return false;
	}
	return true;
}

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

	void insert_elements(ssmr_t& container, entity first, entity last)
	{
		auto start_count_added	 = container.indices(ssmr_t::stage_range_t::ADDED).size();
		auto start_count_settled = container.indices(ssmr_t::stage_range_t::SETTLED).size();
		float default_value		 = 10.0f;
		entity count			 = 0;

		section<"with data">() = [&]() {
			count = last - first;
			for(entity i = first; i < last; ++i, ++default_value)
			{
				container.insert(i, default_value);
			}
		};
		section<"without data (post modification)">() = [&]() {
			count = last - first;
			for(entity i = first; i < last; ++i)
			{
				container.insert(i);
			}
			for(entity i = first; i < last; ++i, ++default_value)
			{
				require(container.set(i, default_value));
			}


			require(all_of_n(first, last, [&default_value, &container](auto i) mutable {
				return container.at<float>(i, ssmr_t::stage_range_t::ADDED) == default_value++;
			}));
		};

		default_value -= count;
		require(all_of_n(first, first + count, [&default_value, &container](auto i) mutable {
			return container.at<float>(i, ssmr_t::stage_range_t::ADDED) == default_value++;
		}));

		require(container.indices(ssmr_t::stage_range_t::ADDED).size()) == count + start_count_added;
		require(container.indices(ssmr_t::stage_range_t::SETTLED).size()) == start_count_settled;
		require(container.indices().size()) == start_count_settled;

		compare_ranges(container);
	}

	void remove_elements(ssmr_t& container, entity first, entity last)
	{
		auto start_count_all	 = container.indices(ssmr_t::stage_range_t::ALL).size();
		auto start_count_alive	 = container.indices(ssmr_t::stage_range_t::ALIVE).size();
		auto start_count_removed = container.indices(ssmr_t::stage_range_t::REMOVED).size();

		size_t erased {0};
		size_t expected = std::accumulate(
		  std::begin(container.indices(ssmr_t::stage_range_t::ALIVE)),
		  std::end(container.indices(ssmr_t::stage_range_t::ALIVE)),
		  size_t {0},
		  [&container](size_t sum, entity index) { return sum + container.has(index, ssmr_t::stage_range_t::ALIVE); });

		section<"ranged">() = [&]() { erased = container.erase(first, last); };

		section<"manual">() = [&]() {
			for(auto i = first; i != last; ++i) erased += container.erase(i);
		};
		require(expected) == erased;

		for(entity i = 0; i < erased; ++i)
		{
			require(!container.has(i + first));
		}
		require(container.indices(ssmr_t::stage_range_t::ALL).size()) == start_count_all;
		require(container.indices(ssmr_t::stage_range_t::ALIVE).size()) == start_count_alive - erased;
		require(container.indices(ssmr_t::stage_range_t::REMOVED).size()) == start_count_removed + erased;

		compare_ranges(container);
	}

	void promote_elements(ssmr_t& container)
	{
		auto added	 = container.indices(ssmr_t::stage_range_t::ADDED).size();
		auto removed = container.indices(ssmr_t::stage_range_t::REMOVED).size();
		auto settled = container.indices(ssmr_t::stage_range_t::SETTLED).size();
		auto all	 = container.indices(ssmr_t::stage_range_t::ALL).size();

		container.promote();

		require(container.indices(ssmr_t::stage_range_t::ADDED).size()) == 0;
		require(container.indices(ssmr_t::stage_range_t::REMOVED).size()) == 0;
		require(container.indices(ssmr_t::stage_range_t::SETTLED).size()) == added + settled;
		require(container.indices(ssmr_t::stage_range_t::ALL).size()) == all - removed;

		compare_ranges(container);
	}

	void merge(ssmr_t& container, ssmr_t& other)
	{
		auto container_indices = psl::array<entity> {container.indices()};
		auto result			   = container.merge(other);
		size_t total {0};
		for(auto index : other.indices())
		{
			require(container.has(index));
			++total;
		}
		require(other.indices().size()) == total;

		auto other_indices = psl::array<entity> {other.indices()};

		psl::array<entity> pre_existing {};
		std::sort(std::begin(container_indices), std::end(container_indices));
		std::sort(std::begin(other_indices), std::end(other_indices));
		std::set_difference(std::begin(container_indices),
							std::end(container_indices),
							std::begin(other_indices),
							std::end(other_indices),
							std::back_inserter(pre_existing));

		require(pre_existing.size()) == container_indices.size() - total;
	}

	constexpr std::array<std::pair<entity, entity>, 3> sizes {{{0, 50}, {0, 5000}, {0, 50000}}};
	constexpr std::array<std::pair<entity, entity>, 3> erase_ranges {{{10, 35}, {4500, 5500}, {10000, 50000}}};

	struct insert_structure
	{
		entity first, last;
	};

	auto t0 = suite<"staged_sparse_memory_region_t::insertion", "ecs", "psl", "collections">(
	  array_typed<insert_structure,
				  insert_structure {0, 50},
				  insert_structure {0, 5000},
				  insert_structure {0, 50000}> {}) =
	  [](insert_structure info) {
		ssmr_t container = ssmr_t::instantiate<float>();
		require(container.indices().size()) >= 0;
		require(container.dense<float>().size()) >= 0;

		auto start_count_added	 = container.indices(ssmr_t::stage_range_t::ADDED).size();
		auto start_count_settled = container.indices(ssmr_t::stage_range_t::SETTLED).size();
		float default_value		 = 10.0f;
		entity count			 = 0;

		section<"with data (single)">(container, info.first, info.last, default_value, count) =
		  [](ssmr_t& container, auto first, auto last, auto default_value, auto& count) {
			  count = last - first;
			  for(entity i = first; i < last; ++i, ++default_value)
			  {
				  container.insert(i, default_value);
			  }
		  };

		section<"with data (ranged)">(container, info.first, info.last, default_value, count) =
		  [](ssmr_t& container, auto first, auto last, auto default_value, auto& count) {
			  count = last - first;
			  std::vector<float> values(size_t {count});
			  std::iota(std::begin(values), std::end(values), default_value);
			  container.insert(first, std::begin(values), std::end(values));
		  };

		section<"without data (post modification)">(container, info.first, info.last, default_value, count) =
		  [](ssmr_t& container, auto first, auto last, auto default_value, auto& count) {
			  count = last - first;
			  for(entity i = first; i < last; ++i)
			  {
				  container.insert(i);
			  }
			  require(all_of_n(first, last, [&default_value, &container](auto i) mutable {
				  return container.set(i, default_value++);
			  }));
		  };

		require(all_of_n(info.first, info.first + count, [&default_value, &container](auto i) mutable {
			return container.at<float>(i, ssmr_t::stage_range_t::ADDED) == default_value++;
		}));

		require(container.indices(ssmr_t::stage_range_t::ADDED).size()) == count + start_count_added;
		require(container.indices(ssmr_t::stage_range_t::SETTLED).size()) == start_count_settled;
		require(container.indices().size()) == start_count_settled;

		compare_ranges(container);

		// section<"inserting several elements">(
		//   val, sizes[index].first, sizes[index].second)
		// section<"inserting several elements">(
		//   val, sizes[index].first, sizes[index].second, erase_ranges[index].first, erase_ranges[index].second) =
		//   [](ssmr_t& container,
		//   entity first,
		//   entity last,
		//   entity erase_first,
		//   entity erase_last) {
		//    insert_elements(container, first, last);
		//    section<"promote elements">(container) = promote_elements;

		//   section<"removing elements">(container, erase_first, erase_last) =
		//	 [](ssmr_t& container, entity first, entity last) {
		//		 remove_elements(container, first, last);
		//		 section<"promote elements">(container) = promote_elements;


		//		 section<"merge">(container, last / 2, last * 2) =
		//		   [](ssmr_t& container, entity first, entity last) {
		//			   ssmr_t other_container = ssmr_t::instantiate<float>();
		//			   for(entity i = first; i < last; ++i)
		//			   {
		//				   other_container.insert(i);
		//			   }
		//			   merge(container, other_container);
		//		   };
		//	 };
		//  };
	};

	struct erase_structure : public insert_structure
	{
		entity erase_first, erase_last;
	};

	auto t1 = suite<"staged_sparse_memory_region_t::erasing", "ecs", "psl", "collections">(
	  array_typed<erase_structure,
				  erase_structure {0, 50, 10, 35},
				  erase_structure {0, 5000, 4500, 5500},
				  erase_structure {0, 50000, 5500, 50000}> {}) = [](erase_structure info) {
		ssmr_t container		 = ssmr_t::instantiate<float>();
		auto start_count_added	 = container.indices(ssmr_t::stage_range_t::ADDED).size();
		auto start_count_settled = container.indices(ssmr_t::stage_range_t::SETTLED).size();

		float default_value = 10.f;
		auto count			= info.last - info.first;
		std::vector<float> values(size_t {count});
		std::iota(std::begin(values), std::end(values), default_value);
		container.insert(info.first, std::begin(values), std::end(values));

		section<"removing elements">(container, info.erase_first, info.erase_last) =
		  [](ssmr_t& container, entity first, entity last) {
			  auto start_count_all	   = container.indices(ssmr_t::stage_range_t::ALL).size();
			  auto start_count_alive   = container.indices(ssmr_t::stage_range_t::ALIVE).size();
			  auto start_count_removed = container.indices(ssmr_t::stage_range_t::REMOVED).size();

			  size_t erased {0};
			  size_t expected {0};
			  for(auto i = first; i < last; ++i)
			  {
				  expected += (size_t)container.has(i);
			  }

			  section<"ranged">() = [&]() { erased = container.erase(first, last); };

			  section<"manual">() = [&]() {
				  for(auto i = first; i != last; ++i) erased += container.erase(i);
			  };
			  require(expected) == erased;


			  require(none_of_n(first, first + erased, [&container](auto index) -> bool { return container.has(index); })) == true;

			  require(container.indices(ssmr_t::stage_range_t::ALL).size()) == start_count_all;
			  require(container.indices(ssmr_t::stage_range_t::ALIVE).size()) == start_count_alive - erased;
			  require(container.indices(ssmr_t::stage_range_t::REMOVED).size()) == start_count_removed + erased;

			  compare_ranges(container);
		  };
	};
}	 // namespace
