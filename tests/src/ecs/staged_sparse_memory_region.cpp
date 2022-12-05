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

	template <typename T>
	inline auto
	append_ssmr(ssmr_t& container, std::vector<std::pair<entity, entity>> ranges, std::vector<T> values = {}) -> size_t
	{
		size_t total {0};
		if(values.empty())
		{
			for(auto [first, last] : ranges)
			{
				for(auto it = first; it != last; ++it)
				{
					total += (size_t)(!container.has(it));
					container.insert(it);
				}
			}
		}
		else
		{
			auto val = std::begin(values);
			for(auto [first, last] : ranges)
			{
				for(auto it = first; it != last; ++it)
				{
					total += (size_t)(!container.has(it));
					if(val == std::end(values)) throw std::exception();
					container.insert(it, *val);
					val = std::next(val);
				}
			}
		}
		return total;
	}

	inline auto erase_ssmr(ssmr_t& container, std::vector<std::pair<entity, entity>> ranges) -> size_t
	{
		size_t total {0};
		for(auto [first, last] : ranges)
		{
			total += container.erase(first, last);
		}
		return total;
	}

	struct insert_structure
	{
		entity first, last;
	};

	struct erase_structure : public insert_structure
	{
		entity erase_first, erase_last;
	};

	auto t0 = suite<ssmr_t, "collections">(array_typed<insert_structure,
													   insert_structure {0, 50},
													   insert_structure {0, 5000},
													   insert_structure {0, 50000}> {}) = [](insert_structure info) {
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
	};

	auto t1 =
	  suite<ssmr_t, "collections">(array_typed<erase_structure,
											   erase_structure {0, 50, 10, 35},
											   erase_structure {0, 5000, 4500, 5500},
											   erase_structure {0, 50000, 5500, 50000}> {}) = [](erase_structure info) {
		  ssmr_t container		   = ssmr_t::instantiate<float>();
		  auto start_count_added   = container.indices(ssmr_t::stage_range_t::ADDED).size();
		  auto start_count_settled = container.indices(ssmr_t::stage_range_t::SETTLED).size();

		  float default_value = 10.f;
		  auto count		  = info.last - info.first;
		  std::vector<float> values(size_t {count});
		  std::iota(std::begin(values), std::end(values), default_value);
		  container.insert(info.first, std::begin(values), std::end(values));

		  section<"removing elements">(container, info.erase_first, info.erase_last) =
			[](ssmr_t& container, entity first, entity last) {
				auto start_count_all	 = container.indices(ssmr_t::stage_range_t::ALL).size();
				auto start_count_alive	 = container.indices(ssmr_t::stage_range_t::ALIVE).size();
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
				require(none_of_n(
				  first, first + erased, [&container](auto index) -> bool { return container.has(index); })) == true;

				require(container.indices(ssmr_t::stage_range_t::ALL).size()) == start_count_all;
				require(container.indices(ssmr_t::stage_range_t::ALIVE).size()) == start_count_alive - erased;
				require(container.indices(ssmr_t::stage_range_t::REMOVED).size()) == start_count_removed + erased;

				compare_ranges(container);
			};
	  };

	auto t2 = suite<ssmr_t, "collections">() = []() {
		auto promote = [](ssmr_t& container) mutable {
			auto previous_added	  = container.indices(ssmr_t::stage_range_t::ADDED).size();
			auto previous_removed = container.indices(ssmr_t::stage_range_t::REMOVED).size();
			auto previous_settled = container.indices(ssmr_t::stage_range_t::SETTLED).size();

			container.promote();

			require(container.indices(ssmr_t::stage_range_t::ADDED).size()) == 0;
			require(container.indices(ssmr_t::stage_range_t::REMOVED).size()) == 0;
			require(container.indices(ssmr_t::stage_range_t::SETTLED).size()) == previous_settled + previous_added;
			require(container.indices(ssmr_t::stage_range_t::ALIVE).size()) == previous_added + previous_settled;
			require(container.indices(ssmr_t::stage_range_t::ALL).size()) == previous_added + previous_settled;
			require(container.indices(ssmr_t::stage_range_t::TERMINAL).size()) == 0;
		};

		ssmr_t container = ssmr_t::instantiate<float>();
		append_ssmr<float>(container, {{15, 50}, {200, 750}});
		promote(container);
		append_ssmr<float>(container, {{900, 1050}, {2000, 7500}});
		erase_ssmr(container, {{0, 1000}, {4000, 5000}});
		promote(container);
	};

	auto t3 = suite<ssmr_t, "collections">() = []() {
		auto merge = [](ssmr_t& container, ssmr_t& other) {
			auto container_indices = psl::array<entity> {container.indices(ssmr_t::stage_range_t::ALL)};

			size_t pre_existing {0};
			for(auto index : other.indices(ssmr_t::stage_range_t::ALL))
			{
				pre_existing += size_t {container.has(index)};
			}

			auto result = container.merge(other);
			for(auto index : other.indices(ssmr_t::stage_range_t::ALL))
			{
				require(container.has(index));
			}

			require(container.indices(ssmr_t::stage_range_t::ALL).size()) ==
			  container_indices.size() + other.indices(ssmr_t::stage_range_t::ALL).size() - pre_existing;
		};

		section<"non-overlapping">() = [&merge]() {
			ssmr_t container_1 = ssmr_t::instantiate<float>();
			append_ssmr<float>(container_1, {{15, 50}, {200, 750}});

			ssmr_t container_2 = ssmr_t::instantiate<float>();
			append_ssmr<float>(container_2, {{1500, 5000}});

			merge(container_1, container_2);
		};

		section<"fully-overlapping">() = [&merge]() {
			ssmr_t container_1 = ssmr_t::instantiate<float>();
			append_ssmr<float>(container_1, {{15, 50}, {200, 750}});

			ssmr_t container_2 = ssmr_t::instantiate<float>();
			append_ssmr<float>(container_2, {{0, 1000}});

			merge(container_1, container_2);
		};


		section<"partial-overlapping">() = [&merge]() {
			ssmr_t container_1 = ssmr_t::instantiate<float>();
			append_ssmr<float>(container_1, {{15, 50}, {200, 750}});

			ssmr_t container_2 = ssmr_t::instantiate<float>();
			append_ssmr<float>(container_2, {{60, 500}});

			merge(container_1, container_2);
		};
	};

	auto t4 = suite<ssmr_t, "collections">() = []() {
		ssmr_t container = ssmr_t::instantiate<float>();
		append_ssmr<float>(container, {{15, 50}, {200, 750}});

		require(container.indices(ssmr_t::stage_range_t::ALL).size()) == 585;

		for(auto index : container.indices(ssmr_t::stage_range_t::ALL))
		{
			container.set(index, (float)index);
		}
		require(std::all_of(std::begin(container.indices(ssmr_t::stage_range_t::ALL)),
							std::end(container.indices(ssmr_t::stage_range_t::ALL)),
							[&container](auto index) { return container.get<float>(index) == (float)index; }));

		psl::sparse_array<entity, entity> sparse {};

		for(entity i = 0; i < 35; ++i)
		{
			sparse[i + 15] = 750 + i;
		}

		container.remap(sparse, [](auto index) -> bool { return index <= 50; });

		require(all_of_n(200, 785, [&container](auto index) { return container.has(index); }));
		require(all_of_n(200, 750, [&container](auto index) { return container.get<float>(index) == (float)index; }));
		require(
		  all_of_n(750, 785, [&container](auto index) { return container.get<float>(index) == (float)(index + 750); }));

		require(container.indices(ssmr_t::stage_range_t::ALL).size()) == 585;
	};
}	 // namespace
