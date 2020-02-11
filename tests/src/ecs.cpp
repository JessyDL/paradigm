#include "stdafx_tests.h"
#include "ecs.h"
#include "psl/ecs/state.h"

using namespace psl::ecs;
using namespace tests::ecs;


void registration_test(info& info) {}


namespace tests::ecs
{
	void float_iteration_test(info& info, psl::ecs::pack<partial, const float, int> pack)
	{
		for(auto [fl, i] : pack)
		{
			i += 5;
		}
	};

	struct object_test
	{
		void empty_system(info& info){};
	};
} // namespace tests::ecs

template <typename T>
auto to_num_string(T i)
{
	static_assert(std::is_arithmetic_v<T>);
	auto conversion = std::to_string(i);
	auto it			= std::end(conversion);
	while(std::distance(std::begin(conversion), it) > 3)
	{
		it = std::prev(it, 3);
		conversion.insert(it, '.');
	}
	return conversion;
}

#define ENABLE_TESTS
//#define BENCHMARK_BASELINE
#define BENCHMARK_ENTITY_CREATION
#define BENCHMARK_COMPONENT_CREATION
#define BENCHMARK_FILTERING
/// -------------------------------------------------------------------------------------------------------------------
/// -- TESTS
/// -------------------------------------------------------------------------------------------------------------------

#ifdef ENABLE_TESTS
TEST_CASE("component_info", "[ECS]")
{
	SECTION("non-empty component_info_typed")
	{
		details::component_info_typed<float> cInfo;

		SECTION("additions")
		{
			psl::array<entity> entities;
			entities.resize(100);
			std::iota(std::begin(entities), std::end(entities), entity{0});
			cInfo.add(entities);
			REQUIRE(cInfo.size() == entities.size());
			REQUIRE(cInfo.added_entities().size() == entities.size());
			std::for_each(std::begin(entities), std::end(entities),
						  [&cInfo](entity e) { cInfo.set(e, static_cast<float>(e)); });
			std::for_each(std::begin(entities), std::end(entities), [&cInfo](entity e) {
				REQUIRE(cInfo.has_component(e));
				REQUIRE(cInfo.has_added(e));
				REQUIRE(cInfo.entity_data().at(e) == static_cast<float>(e));
			});

			SECTION("removals")
			{
				std::random_device rd;
				std::mt19937 g(rd());
				std::shuffle(std::begin(entities), std::end(entities), g);

				auto count = entities.size() / 10;
				for(auto c = 0; c < 10; ++c)
				{
					for(auto i = 0; i < count; ++i)
					{
						auto index = c * 10 + i;
						cInfo.destroy(entities[index]);
					}

					for(entity i = 0; i < static_cast<entity>(entities.size()); ++i)
					{
						if(i < (c + 1) * 10)
						{
							auto index = entities[i];
							REQUIRE(!cInfo.has_component(index));
							REQUIRE(cInfo.has_removed(index));
							REQUIRE(cInfo.entity_data().at(index, 2, 2) == static_cast<float>(index));
						}
						else
						{
							auto index = entities[i];
							REQUIRE(cInfo.has_component(index));
							REQUIRE(cInfo.entity_data().at(index) == static_cast<float>(index));
						}
					}
				}
			}
		}


		SECTION("additions && removals") {}

		SECTION("remap")
		{
			details::component_info_typed<float> cInfo2;
			psl::array<entity> entities;
			entities.resize(100);
			std::iota(std::begin(entities), std::end(entities), entity{0});
			cInfo.add(entities);
			cInfo2.add(entities);

			std::for_each(std::begin(entities), std::end(entities),
						  [&cInfo](entity e) { cInfo.set(e, static_cast<float>(e)); });

			std::for_each(std::begin(entities), std::end(entities),
						  [&cInfo2, offset = static_cast<entity>(cInfo.size())](entity e) {
							  cInfo2.set(e, static_cast<float>(e + offset));
						  });

			psl::sparse_array<entity> remap;
			std::for_each(std::begin(entities), std::end(entities),
						  [&remap, offset = static_cast<entity>(cInfo.size())](entity e) { remap[e] = e + offset; });
			cInfo2.remap(remap, [offset = static_cast<entity>(cInfo.size())](entity e) { return e < offset; });
			std::for_each(std::begin(cInfo2.entities()), std::end(cInfo2.entities()),
						  [offset = static_cast<entity>(cInfo.size())](entity e) { REQUIRE(e >= offset); });

			SECTION("merge")
			{
				auto orig_size = cInfo.size();
				cInfo.merge(cInfo2);
				REQUIRE(cInfo.size() == orig_size + cInfo2.size());
				std::for_each(std::begin(cInfo.entities()), std::end(cInfo.entities()),
							  [&cInfo, offset = static_cast<entity>(cInfo.size())](entity e) {
								  REQUIRE(e <= offset);
								  REQUIRE(cInfo.entity_data()[e] == static_cast<float>(e));
							  });
			}
		}
	}

	SECTION("empty component_info_typed") {}
}

TEST_CASE("component_key must be unique", "[ECS]")
{
	using namespace psl::ecs::details;
	auto fl_id	= component_key<float>;
	auto int_id = component_key<int>;
	REQUIRE(fl_id != int_id);


	constexpr auto fl_cid  = component_key<float>;
	constexpr auto int_cid = component_key<int>;
	REQUIRE(fl_cid != int_cid);


	REQUIRE(fl_cid == fl_id);
	REQUIRE(int_cid == int_id);
}

TEST_CASE("filtering", "[ECS]")
{
	state state;
	auto e_list1{state.create(100)};
	auto e_list2{state.create(400)};
	auto e_list3{state.create(500)};

	SECTION("only the first 100 are given all components")
	{
		state.add_components<float>(e_list1);
		state.add_components<size_t>(e_list1);
		state.add_components<int>(e_list1);
		state.add_components<char>(e_list1);
		state.add_components<std::byte>(e_list1);

		auto f = state.filter<float, size_t, int, char, std::byte>();
		REQUIRE(f == e_list1);
		REQUIRE(state.filter<float>().size() == state.filter<on_add<float>>().size());
		REQUIRE(state.filter<float>().size() == state.filter<on_combine<float, size_t>>().size());
		REQUIRE(state.filter<on_remove<float>>().size() == state.filter<on_break<float, size_t>>().size());
	}

	SECTION("500 are given two component types and 100 are given 3 component types where 2 overlap")
	{
		state.add_components<int>(e_list1);
		state.add_components<char>(e_list1);
		state.add_components<size_t>(e_list1);

		state.add_components<char>(e_list3);
		state.add_components<size_t>(e_list3);


		auto f = state.filter<float, size_t, int, char, std::byte>();
		REQUIRE(f.size() == 0);

		f = state.filter<size_t, int, char>();
		REQUIRE(f == e_list1);

		f								= state.filter<size_t, char>();
		std::vector<entity> combination = e_list1;
		combination.reserve(e_list1.size() + e_list3.size());
		combination.insert(std::end(combination), std::begin(e_list3), std::end(e_list3));
		REQUIRE(f == combination);
		f = state.filter<char>();
		REQUIRE(f == combination);
	}

	SECTION("filtering components that are non-contiguous")
	{
		state.create(500, float{}, size_t{});
		state.destroy(1200);
		auto entities = state.create(3, float{}, size_t{});

		REQUIRE(entities.size() == 3);
		REQUIRE(entities[0] == 1500);
		REQUIRE(entities[1] == 1501);
		REQUIRE(entities[2] == 1502);
		REQUIRE(state.filter<float>().size() == 502); // 500 + 3 - 1
		REQUIRE(state.filter<float>().size() == state.filter<on_add<float>>().size());
		REQUIRE(state.filter<float>().size() == state.filter<on_combine<float, size_t>>().size());
		REQUIRE(state.filter<on_remove<float>>().size() == 1); // we deleted entity 1200
		REQUIRE(state.filter<on_remove<float>>().size() == state.filter<on_break<float, size_t>>().size());
	}
}

struct position
{
	size_t x;
	size_t y;
};

TEST_CASE("initializing components", "[ECS]")
{
	state state;

	size_t count{0};
	state.create(
		50,
		[&count](position& i) {
			i = {++count, 0};
		},
		float{5.0f}, psl::ecs::empty<size_t>());

	REQUIRE(state.view<position>().size() == 50);
	REQUIRE(state.view<float>().size() == 50);
	REQUIRE(state.view<size_t>().size() == 50);

	count = {0};
	size_t check{0};
	size_t check_view{0};
	for(const auto& i : state.view<position>())
	{
		++count;
		check += count;
		check_view += i.x;
	}

	REQUIRE(check_view == check);
}


TEST_CASE("systems", "[ECS]")
{
	state state;

	auto group = details::make_filter_group(psl::templates::type_container<pack<entity, int>>{});


	SECTION("lifetime test")
	{
		auto e_list1{state.create(10)};
		auto e_list2{state.create(40)};
		auto e_list3{state.create(50)};
		// pre-tick #1
		// we add int components to all elements in e_list1, by giving them an incrementing value
		// thanks to these being the first entities, they overlap with their ID
		auto incrementer = 0;
		state.add_components(e_list1, [&incrementer](int& target) { target = incrementer++; });

		state.declare(
			[](info& info, pack<entity, filter<int>> pack) { info.command_buffer.destroy(pack.get<entity>()); });
		auto token = state.declare([size_1 = e_list1.size()](info& info, pack<entity, const int> pack1) {
			for(auto [e, i] : pack1)
			{
				REQUIRE(e == i);
			}
			REQUIRE(pack1.size() == size_1);
		});

		REQUIRE(e_list1.size() == state.filter<on_add<int>>().size());
		REQUIRE(e_list1.size() == state.filter<int>().size());

		// tick #1
		// here we verify the resources of e_list1 are all present, and their values accurate
		// followed by deleting them all.
		state.tick(std::chrono::duration<float>(0.1f));
		{
			for(auto e : e_list1)
			{
				auto val = state.get<int>(e);
				REQUIRE(e == val);
			}
		}
		state.revoke(token);

		// pre-tick #2
		// we add int components to all elements in e_list2, by giving them an incrementing value with offset of
		// elist1.size(), thanks to these being the first entities, they overlap with their ID
		REQUIRE(e_list1.size() == state.filter<on_remove<int>>().size());
		REQUIRE(0 == state.filter<int>().size());
		incrementer = e_list1.size();
		state.add_components(e_list2, [&incrementer](int& target) { target = incrementer++; });
		REQUIRE(e_list2.size() == state.filter<on_add<int>>().size());
		REQUIRE(e_list2.size() == state.filter<int>().size());
		token = state.declare([size_1 = e_list1.size(),
							   size_2 = e_list2.size()](info& info, pack<entity, const int, on_remove<int>> pack1,
														pack<entity, const int, filter<int>> pack2) {
			for(auto [e, i] : pack1)
			{
				REQUIRE(e == i);
			}
			for(auto [e, i] : pack2)
			{
				REQUIRE(e == i);
			}
			REQUIRE(pack1.size() == size_1);
			REQUIRE(pack2.size() == size_2);
		});

		// tick #2
		// we verify the elements of e_list1 are deleted and their data is intact
		// we verify the elements of e_list2 are added and their data is correct
		// we also remove all elements of e_list2
		state.tick(std::chrono::duration<float>(0.1f));
		state.revoke(token);

		// pre-tick #3
		// we verify that no int component is present anymore in the system aside from the previously removed ones
		REQUIRE(e_list2.size() == state.filter<on_remove<int>>().size());
		REQUIRE(0 == state.filter<int>().size());
		token = state.declare(
			[size_2 = e_list2.size()](info& info, pack<entity, on_remove<int>> pack1, pack<entity, filter<int>> pack2) {
				REQUIRE(pack1.size() == size_2);
				REQUIRE(pack2.size() == 0);
			});

		// tick #3
		state.tick(std::chrono::duration<float>(0.1f));
		state.revoke(token);
		token = state.declare([](info& info, pack<entity, on_remove<int>> pack1, pack<entity, filter<int>> pack2) {
			REQUIRE(pack1.size() == 0);
			REQUIRE(pack2.size() == 0);
		});

		// tick #4
		state.tick(std::chrono::duration<float>(0.1f));

		// tick #5
		state.tick(std::chrono::duration<float>(0.1f));

		REQUIRE(0 == state.filter<on_remove<int>>().size());
		REQUIRE(0 == state.filter<int>().size());
	}

	SECTION("continuous removal from within systems")
	{
		auto e_list2{state.create(40)};
		psl::array<int> values;
		values.resize(e_list2.size());
		std::iota(std::begin(values), std::end(values), 0);
		;
		state.add_components<int>(e_list2, values);
		auto expected = e_list2.size();

		state.declare([&expected](info& info, pack<entity, int> pack) {
			REQUIRE(pack.size() == expected);
			psl::array<entity> entities;
			for(auto [e, i] : pack)
			{
				REQUIRE(static_cast<int>(e) == i);
				if(std::rand() % 2 == 0)
				{
					entities.emplace_back(e);
					--expected;
				}
			}
			info.command_buffer.remove_components<int>(entities);
		});

		while(expected > 0) state.tick(std::chrono::duration<float>(0.1f));
	}


	SECTION("continuous removal from external")
	{
		auto e_list2{state.create(40)};
		psl::array<int> values;
		values.resize(e_list2.size());
		std::iota(std::begin(values), std::end(values), 0);
		state.add_components<int>(e_list2, values);
		auto expected = e_list2.size();

		state.declare([&expected](info& info, pack<entity, int> pack) {
			REQUIRE(pack.size() == expected);
			for(auto [e, i] : pack)
			{
				REQUIRE(static_cast<int>(e) == i);
			}
		});

		while(expected > 0)
		{
			state.tick(std::chrono::duration<float>(0.1f));

			auto mid = std::partition(std::begin(e_list2), std::end(e_list2), [](auto e) { return std::rand() % 2; });
			state.remove_components<int>(
				psl::array_view<entity>{mid, static_cast<size_t>(std::distance(mid, std::end(e_list2)))});
			expected -= std::distance(mid, std::end(e_list2));
			e_list2.erase(mid, std::end(e_list2));
		}
	}

	SECTION("continuous addition from within systems")
	{
		auto e_list2{state.create(40)};
		psl::array<int> values;
		values.resize(e_list2.size());
		std::iota(std::begin(values), std::end(values), 0);
		state.add_components<int>(e_list2, values);
		auto expected = e_list2.size();

		state.declare([&expected](info& info, pack<entity, int> pack) {
			REQUIRE(pack.size() == expected);

			for(auto [e, i] : pack)
			{
				REQUIRE(static_cast<int>(e) == i);
			}
			auto new_count				= std::rand() % 20;
			psl::array<entity> entities = info.command_buffer.create(new_count);
			psl::array<int> values;
			values.resize(entities.size());
			std::iota(std::begin(values), std::end(values), static_cast<int>(expected));
			info.command_buffer.add_components<int>(entities, values);
			expected += new_count;
		});

		while(expected <= 10'000) state.tick(std::chrono::duration<float>(0.1f));
	}


	SECTION("continuous addition from external")
	{
		auto e_list2{state.create(40)};
		{
			psl::array<int> values;
			values.resize(e_list2.size());
			std::iota(std::begin(values), std::end(values), 0);
			state.add_components<int>(e_list2, values);
		}
		auto expected = e_list2.size();

		state.declare([&expected](info& info, pack<entity, int> pack) {
			REQUIRE(pack.size() == expected);

			for(auto [e, i] : pack)
			{
				REQUIRE(static_cast<int>(e) == i);
			}
		});

		while(expected <= 10'000)
		{
			state.tick(std::chrono::duration<float>(0.1f));

			auto new_count				= std::rand() % 20;
			psl::array<entity> entities = state.create(new_count);
			psl::array<int> values;
			values.resize(entities.size());
			std::iota(std::begin(values), std::end(values), static_cast<int>(expected));
			state.add_components<int>(entities, values);
			expected += new_count;
		}
	}

	SECTION("simple iterations test")
	{
		auto e_list1{state.create(10)};
		auto e_list2{state.create(40)};
		auto e_list3{state.create(50)};
		state.add_components<float>(e_list1);
		state.add_components<int>(e_list1);
		auto system_id = state.declare(float_iteration_test);
		for(int i = 0; i < 10; ++i) state.tick(std::chrono::duration<float>(0.1f));

		auto entities = state.filter<int>();
		auto results  = state.view<int>();
		REQUIRE(results.size() == entities.size());
		REQUIRE(results.size() == e_list1.size());
		REQUIRE(std::all_of(std::begin(results), std::end(results), [](const auto& res) { return res == 50; }));

		REQUIRE(state.systems() == 1);
		state.revoke(system_id);
		REQUIRE(state.systems() == 0);
		state.tick(std::chrono::duration<float>(0.1f));
		REQUIRE(std::all_of(std::begin(results), std::end(results), [](const auto& res) { return res == 50; }));
	}
}


TEST_CASE("declaring system signatures", "[ECS]")
{
	state state;
	state.declare(registration_test);
	object_test test;
	state.declare(&object_test::empty_system, &test);
	state.declare([](psl::ecs::info& info) {});
}
#endif

/// -------------------------------------------------------------------------------------------------------------------
/// -- BENCHMARKS
/// -------------------------------------------------------------------------------------------------------------------
#ifdef BENCHMARK_BASELINE
TEST_CASE("sparse_array", "[ECS]")
{

	auto eCount		= GENERATE(1, 1'000, 10'000, 1'000'000);
	auto eCount_str = to_num_string(eCount);

	BENCHMARK("array insert " + eCount_str)
	{
		psl::array<float> array;
		for(auto i = 0; i < eCount; ++i) array.emplace_back(0.0f);
		return array;
	};

	BENCHMARK("sparse_array insert " + eCount_str)
	{
		psl::sparse_array<float> sparse;
		for(auto i = 0; i < eCount; ++i) sparse.insert(i, 0.0f);
		return sparse;
	};

	BENCHMARK("memory::sparse_array insert " + eCount_str)
	{
		memory::sparse_array<float> sparse;
		for(auto i = 0; i < eCount; ++i) sparse.insert(i, 0.0f);
		return sparse;
	};

	BENCHMARK("sparse_indice_array insert " + eCount_str)
	{
		psl::sparse_indice_array<size_t> sparse;
		for(auto i = 0; i < eCount; ++i) sparse.insert(i);
		return sparse;
	};
}
#endif
#ifdef BENCHMARK_ENTITY_CREATION
TEST_CASE("Entity creation", "[ECS]")
{
	state state;

	auto eCount		= GENERATE(1, 1'000, 1'000'000);
	auto eCount_str = to_num_string(eCount);
	BENCHMARK_ADVANCED("creating " + eCount_str + " entities")(Catch::Benchmark::Chronometer meter)
	{
		state.clear();
		meter.measure([&]() { return state.create(eCount); });
	};

	if(eCount <= 1) return;

	auto eHalfCount		= eCount / 2;
	auto eHalfCount_str = to_num_string(eHalfCount);

	BENCHMARK_ADVANCED("creating " + eCount_str + " entities, destroying first " + eHalfCount_str + ", creating " +
					   eHalfCount_str)
	(Catch::Benchmark::Chronometer meter)
	{
		state.clear();
		meter.measure([&]() {
			state.create(eCount);
			state.destroy({{std::pair{0, eHalfCount}}});
			state.tick(std::chrono::duration<float>{0.f});
			return state.create(eHalfCount);
		});
	};
}
#endif
#ifdef BENCHMARK_COMPONENT_CREATION
struct malloc_test
{
	malloc_test(size_t components, size_t count)
	{
		if(components >= 5) arr1.resize(count);
		if(components >= 4) arr2.resize(count);
		if(components >= 3) arr3.resize(count);
		if(components >= 2) arr4.resize(count);
		arr5.resize(count);
	}

	std::vector<uint64_t> arr1{};
	std::vector<bool> arr2{};
	std::vector<char> arr3{};
	std::vector<float> arr4{};
	std::vector<int> arr5{};
};

TEST_CASE("Component creation", "[ECS]")
{
	state state;
	auto cCount		= GENERATE(1, 2, 5);
	auto cCount_str = to_num_string(cCount);

	auto eCount		= GENERATE(1, 1'000, 1'000'000);
	auto eCount_str = to_num_string(eCount);
	BENCHMARK("malloc " + cCount_str + " component to " + eCount_str + " entities")
	{
		return malloc_test(cCount, eCount);
	};

	BENCHMARK_ADVANCED("add " + cCount_str + " component to " + eCount_str + " entities")
	(Catch::Benchmark::Chronometer meter)
	{
		state.clear();
		std::vector<std::vector<entity>> entities_arr(meter.runs());
		for(auto i = 0; i < meter.runs(); ++i) entities_arr[i] = state.create(eCount);
		meter.measure([&](int i) {
			if(cCount >= 5) state.add_components<uint64_t>(entities_arr[i]);
			if(cCount >= 4) state.add_components<bool>(entities_arr[i]);
			if(cCount >= 3) state.add_components<char>(entities_arr[i]);
			if(cCount >= 2) state.add_components<float>(entities_arr[i]);

			return state.add_components<int>(entities_arr[i]);
		});
	};
	// std::abort();
}

#endif
using gen = Catch::Generators::FixedValuesGenerator<int>;
#ifdef BENCHMARK_FILTERING
TEST_CASE("Filtering", "[ECS]")
{
	auto generator	= GENERATE(gen{10'000, 300, 2'700, 1'200, 6'700}, gen{100'000, 3'000, 21'700, 10'200, 68'700},
							   gen{10'000, 300, 2'700, 3'200, 6'700}, gen{100'000, 3'000, 21'700, 30'200, 68'700});
	auto eCount		= generator.get();
	auto eCount_str = to_num_string(eCount);
	generator.next();
	auto char_beg = generator.get();
	generator.next();
	auto char_end = generator.get();
	generator.next();
	auto int_beg = generator.get();
	generator.next();
	auto int_end = generator.get();

	state state;
	std::ignore = state.create(eCount);

	state.add_components<float>({{std::pair{0, eCount}}});
	state.add_components<char>({{std::pair{char_beg, char_end}}});

	state.add_components({{std::pair{int_beg, int_end}}}, [](int& i) { i = std::rand() % 1000; });

	BENCHMARK("filter " + to_num_string(int_end - int_beg) + " ints from " + eCount_str + " entities")
	{
		return state.filter<int>();
	};

	BENCHMARK("filter on <char, float> (" + to_num_string(char_end - char_beg) + ") from " + eCount_str + " entities")
	{
		return state.filter<char, float>();
	};

	BENCHMARK("filter on <char, int> (" + to_num_string(std::max(0, int_beg - char_end)) + ") from " + eCount_str +
			  " entities")
	{
		return state.filter<char, int>();
	};

	BENCHMARK("filter on <char, int, float> (" + to_num_string(std::max(0, int_beg - char_end)) + ") from " +
			  eCount_str + " entities")
	{
		return state.filter<char, int, float>();
	};
	BENCHMARK("filter on <float, int, char> (" + to_num_string(std::max(0, int_beg - char_end)) + ") from " +
			  eCount_str + " entities")
	{
		return state.filter<float, int, char>();
	};

	// order_by
	BENCHMARK_ADVANCED("order_by on  " + to_num_string(int_end - int_beg) + " entities")
	(Catch::Benchmark::Chronometer meter)
	{
		auto entities = state.filter<int>();
		std::vector<std::vector<entity>> entities_arr(meter.runs());
		for(auto i = 0; i < meter.runs(); ++i) entities_arr[i] = entities;
		meter.measure([&](int i) {
			state.order_by<std::less<int>, int>(std::begin(entities_arr[i]), std::end(entities_arr[i]));
			return entities_arr[i];
		});
	};

	// conditional
	BENCHMARK_ADVANCED("on_condition on  " + to_num_string(int_end - int_beg) + " entities")
	(Catch::Benchmark::Chronometer meter)
	{
		auto entities = state.filter<int>();
		std::vector<std::vector<entity>> entities_arr(meter.runs());
		for(auto i = 0; i < meter.runs(); ++i) entities_arr[i] = entities;

		meter.measure([&](int i) {
			state.on_condition<int>(std::begin(entities_arr[i]), std::end(entities_arr[i]),
									[](const int& i) { return i < 500; });
			return entities_arr[i];
		});
	};
}
#endif

template <typename T, typename... Ts>
auto read_only_system = [](info& info, pack<T, const Ts...>) {};

template <typename T, typename... Ts>
auto write_system = [](info& info, pack<T, Ts...>) {};

auto get_random_entities(const psl::array<entity>& source, size_t count, std::mt19937 g)
{
	auto copy = source;
	std::shuffle(std::begin(copy), std::end(copy), g);
	copy.resize(std::min(copy.size(), count));
	return copy;
}

template <typename... Ts>
void systems_test(psl::array<entity> count)
{
	state state;
	auto entities	= state.create(count[0]);
	auto eCount_str = to_num_string(entities.size());
	std::random_device rd;
	std::mt19937 g(rd());
	size_t i{1};
	(state.add_components<Ts>(get_random_entities(entities, count[i++], g)), ...);
	BENCHMARK("sequential - read-only (" + eCount_str + ")")
	{
		auto system = state.declare(threading::seq, read_only_system<full, Ts...>);
		state.tick(std::chrono::duration<float>{0.0f});
		state.revoke(system);
	};

	BENCHMARK("sequential - read-only (2 ticks) (" + eCount_str + ")")
	{
		auto system = state.declare(threading::seq, read_only_system<full, Ts...>);
		state.tick(std::chrono::duration<float>{0.0f});
		state.tick(std::chrono::duration<float>{0.0f});
		state.revoke(system);
	};

	BENCHMARK("sequential - write (" + eCount_str + ")")
	{
		auto system = state.declare(threading::seq, write_system<full, Ts...>);
		state.tick(std::chrono::duration<float>{0.0f});
		state.revoke(system);
	};

	BENCHMARK("sequential - write (2 ticks) (" + eCount_str + ")")
	{
		auto system = state.declare(threading::seq, write_system<full, Ts...>);
		state.tick(std::chrono::duration<float>{0.0f});
		state.tick(std::chrono::duration<float>{0.0f});
		state.revoke(system);
	};

	BENCHMARK("parallel - read-only (" + eCount_str + ")")
	{
		auto system = state.declare(threading::par, read_only_system<partial, Ts...>);
		state.tick(std::chrono::duration<float>{0.0f});
		state.revoke(system);
	};

	BENCHMARK("parallel - read-only (2 ticks) (" + eCount_str + ")")
	{
		auto system = state.declare(threading::par, read_only_system<partial, Ts...>);
		state.tick(std::chrono::duration<float>{0.0f});
		state.tick(std::chrono::duration<float>{0.0f});
		state.revoke(system);
	};

	BENCHMARK("parallel - write (" + eCount_str + ")")
	{
		auto system = state.declare(threading::par, write_system<partial, Ts...>);
		state.tick(std::chrono::duration<float>{0.0f});
		state.revoke(system);
	};

	BENCHMARK("parallel - write (2 ticks) (" + eCount_str + ")")
	{
		auto system = state.declare(threading::par, write_system<partial, Ts...>);
		state.tick(std::chrono::duration<float>{0.0f});
		state.tick(std::chrono::duration<float>{0.0f});
		state.revoke(system);
	};
}

#include "ecs/components/transform.h"
#include "ecs/components/lifetime.h"
#include "ecs/components/velocity.h"
#include "ecs/components/camera.h"


TEST_CASE("Systems", "[ECS]")
{
	auto generator =
		GENERATE(gen{10'000, 300, 2'700, 1'200, 6'700}, gen{100'000, 3'000, 21'700, 10'200, 68'700},
				 gen{1'000'000, 3'000, 20'700, 30'200, 60'700}, gen{1'000'000, 300'000, 210'700, 300'200, 680'700});

	psl::array<entity> counts;
	do
	{
		counts.emplace_back(generator.get());
	} while(generator.next());

	SECTION("trivial types") { systems_test<char, int, float, uint64_t>(counts); }

	SECTION("complex types")
	{
		using namespace core::ecs::components;
		systems_test<camera, velocity, lifetime, transform>(counts);
	}
}