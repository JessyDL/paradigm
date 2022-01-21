#include "ecs.hpp"
#include "psl/ecs/state.hpp"
#include "stdafx_tests.hpp"
#include <random>

using namespace psl::ecs;
using namespace tests::ecs;

void registration_test(psl::ecs::info_t& info) {}


namespace tests::ecs
{
	void float_iteration_test(psl::ecs::info_t& info, psl::ecs::pack<partial, const float, int> pack)
	{
		for(auto [fl, i] : pack)
		{
			i += 5;
		}
	};

	struct object_test
	{
		void empty_system(psl::ecs::info_t& info) {};
	};
}	 // namespace tests::ecs

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


#include <litmus/expect.hpp>
#include <litmus/section.hpp>
#include <litmus/suite.hpp>

using namespace litmus;

namespace
{
	struct position
	{
		size_t x;
		size_t y;
	};

	template <typename T>
	struct wrapper_t
	{
		T value {};
	};

	auto t0 = suite<"component_info", "ecs", "psl">() = []() {
		section<"non-empty component_info_typed">() = [&]() {
			details::component_info_typed<float> cInfo;

			section<"additions">() = [&]() {
				psl::array<entity> entities;
				entities.resize(100);
				std::iota(std::begin(entities), std::end(entities), entity {0});
				cInfo.add(entities);
				require(cInfo.size()) == entities.size();
				require(cInfo.added_entities().size()) == entities.size();
				std::for_each(std::begin(entities), std::end(entities), [&cInfo](entity e) {
					cInfo.set(e, static_cast<float>(e));
				});
				std::for_each(std::begin(entities), std::end(entities), [&cInfo](entity e) {
					require(cInfo.has_component(e));
					require(cInfo.has_added(e));
					require(cInfo.entity_data().at(e)) == static_cast<float>(e);
				});

				section<"removals">() = [&]() {
					std::random_device rd;
					std::mt19937 g(rd());
					std::shuffle(std::begin(entities), std::end(entities), g);

					auto count = entities.size() / 10;
					for(entity c = 0; c < 10; ++c)
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
								require(!cInfo.has_component(index));
								require(cInfo.has_removed(index));
								require(cInfo.entity_data().at(index, 2, 2)) == static_cast<float>(index);
							}
							else
							{
								auto index = entities[i];
								require(cInfo.has_component(index));
								require(cInfo.entity_data().at(index)) == static_cast<float>(index);
							}
						}
					}
				};
			};


			// section<"additions && removals">() = [&](){};
			section<"remap">() = [&]() {
				details::component_info_typed<float> cInfo2;
				psl::array<entity> entities;
				entities.resize(100);
				std::iota(std::begin(entities), std::end(entities), entity {0});
				cInfo.add(entities);
				cInfo2.add(entities);

				std::for_each(std::begin(entities), std::end(entities), [&cInfo](entity e) {
					cInfo.set(e, static_cast<float>(e));
				});

				std::for_each(std::begin(entities),
							  std::end(entities),
							  [&cInfo2, offset = static_cast<entity>(cInfo.size())](entity e) {
								  cInfo2.set(e, static_cast<float>(e + offset));
							  });

				psl::sparse_array<entity> remap;
				std::for_each(
				  std::begin(entities),
				  std::end(entities),
				  [&remap, offset = static_cast<entity>(cInfo.size())](entity e) { remap[e] = e + offset; });
				cInfo2.remap(remap, [offset = static_cast<entity>(cInfo.size())](entity e) { return e < offset; });
				std::for_each(std::begin(cInfo2.entities()),
							  std::end(cInfo2.entities()),
							  [offset = static_cast<entity>(cInfo.size())](entity e) { require(e) >= offset; });
				section<"merge">() = [&]() {
					auto orig_size = cInfo.size();
					cInfo.merge(cInfo2);
					require(cInfo.size()) == orig_size + cInfo2.size();
					std::for_each(std::begin(cInfo.entities()),
								  std::end(cInfo.entities()),
								  [&cInfo, offset = static_cast<entity>(cInfo.size())](entity e) {
									  require(e) <= offset;
									  require(cInfo.entity_data()[e]) == static_cast<float>(e);
								  });
				};
			};
		};
		// section<"empty component_info_typed">() = [&]() {};
	};

	auto t1 = suite<"component_key must be unique", "ecs", "psl">() = []() {
		using namespace psl::ecs::details;
		auto fl_id	= (std::uintptr_t)component_key<float>;
		auto int_id = (std::uintptr_t)component_key<int>;
		require(fl_id) != int_id;


		constexpr auto fl_cid  = component_key<float>;
		constexpr auto int_cid = component_key<int>;
		require((std::uintptr_t)fl_cid) != (std::uintptr_t)int_cid;


		require((std::uintptr_t)fl_cid) == fl_id;
		require((std::uintptr_t)int_cid) == int_id;
	};

	auto t2 = suite < "filtering", "ecs", "psl">() = []() {
		state_t state;
		auto e_list1 {state.create(100)};
		auto e_list2 {state.create(400)};
		auto e_list3 {state.create(500)};


		section<"only the first 100 are given all components">() = [&]() {
			state.add_components<float>(e_list1);
			state.add_components<size_t>(e_list1);
			state.add_components<int>(e_list1);
			state.add_components<char>(e_list1);
			state.add_components<std::byte>(e_list1);

			auto f = state.filter<float, size_t, int, char, std::byte>();
			require(f.size()) == e_list1.size();
			require(std::equal(std::begin(f), std::end(f), std::begin(e_list1)));
			require(state.filter<float>().size()) == state.filter<on_add<float>>().size();
			require(state.filter<float>().size()) == state.filter<on_combine<float, size_t>>().size();
			require(state.filter<on_remove<float>>().size()) == state.filter<on_break<float, size_t>>().size();
		};

		section<"500 are given two component types and 100 are given 3 component types where 2 overlap">() = [&]() {
			state.add_components<int>(e_list1);
			state.add_components<char>(e_list1);
			state.add_components<size_t>(e_list1);

			state.add_components<char>(e_list3);
			state.add_components<size_t>(e_list3);


			auto f = state.filter<float, size_t, int, char, std::byte>();
			require(f.size()) == 0;

			f = state.filter<size_t, int, char>();
			require(f.size()) == e_list1.size();
			require(std::equal(std::begin(f), std::end(f), std::begin(e_list1)));

			f								= state.filter<size_t, char>();
			std::vector<entity> combination = e_list1;
			combination.reserve(e_list1.size() + e_list3.size());
			combination.insert(std::end(combination), std::begin(e_list3), std::end(e_list3));
			require(std::equal(std::begin(f), std::end(f), std::begin(combination)));
			require(f.size()) == combination.size();
			f = state.filter<char>();
			require(std::equal(std::begin(f), std::end(f), std::begin(combination)));
			require(f.size()) == combination.size();
		};

		section<"filtering components that are non-contiguous">() = [&]() {
			state.create(500, float {}, size_t {});
			state.destroy(1200);
			auto entities = state.create(3, float {}, size_t {});

			require(entities.size()) == 3;
			require(entities[0]) == 1500;
			require(entities[1]) == 1501;
			require(entities[2]) == 1502;
			require(state.filter<float>().size()) == 502;	 // 500 + 3 - 1
			require(state.filter<float>().size()) == state.filter<on_add<float>>().size();
			require(state.filter<float>().size()) == state.filter<on_combine<float, size_t>>().size();
			require(state.filter<on_remove<float>>().size()) == 1;	  // we deleted entity 1200
			require(state.filter<on_remove<float>>().size()) == state.filter<on_break<float, size_t>>().size();
		};
	};

	auto t3 = suite<"initializing components", "ecs", "psl">() = []() {
		state_t state;

		size_t count {0};
		state.create(
		  50,
		  [&count](position& i) {
			  i = {++count, 0};
		  },
		  float {5.0f},
		  psl::ecs::empty<size_t>());

		require(state.view<position>().size()) == 50;
		require(state.view<float>().size()) == 50;
		require(state.view<size_t>().size()) == 50;

		count = {0};
		size_t check {0};
		size_t check_view {0};
		for(const auto& i : state.view<position>())
		{
			++count;
			check += count;
			check_view += i.x;
		}

		require(check_view) == check;
	};

	auto t4 = suite<"systems", "ecs", "psl">() = []() {
		state_t state;

		auto group = details::make_filter_group(psl::templates::type_container<pack<entity, int>> {});


		section<"lifetime test">() = [&]() {
			auto e_list1 {state.create(10)};
			auto e_list2 {state.create(40)};
			auto e_list3 {state.create(50)};
			// pre-tick #1
			// we add int components to all elements in e_list1, by giving them an incrementing value
			// thanks to these being the first entities, they overlap with their ID
			auto incrementer = 0;
			state.add_components(e_list1, [&incrementer](int& target) { target = incrementer++; });

			state.declare([](psl::ecs::info_t& info, pack<entity, filter<int>> pack) {
				info.command_buffer.destroy(pack.get<entity>());
			});
			auto token =
			  state.declare([size_1 = e_list1.size()](psl::ecs::info_t& info, pack<entity, const int> pack1) {
				  for(auto [e, i] : pack1)
				  {
					  require(e) == i;
				  }
				  require(pack1.size()) == size_1;
			  });

			require(e_list1.size()) == state.filter<on_add<int>>().size();
			require(e_list1.size()) == state.filter<int>().size();

			// tick #1
			// here we verify the resources of e_list1 are all present, and their values accurate
			// followed by deleting them all.
			state.tick(std::chrono::duration<float>(0.1f));
			{
				for(auto e : e_list1)
				{
					auto val = state.get<int>(e);
					require(e) == val;
				}
			}
			state.revoke(token);

			// pre-tick #2
			// we add int components to all elements in e_list2, by giving them an incrementing value with offset of
			// elist1.size(), thanks to these being the first entities, they overlap with their ID
			require(e_list1.size()) == state.filter<on_remove<int>>().size();
			require(0) == state.filter<int>().size();
			require(0) == state.filter<on_add<int>>().size();
			incrementer = e_list1.size();
			state.add_components(e_list2, [&incrementer](int& target) { target = incrementer++; });
			require(e_list2.size()) == state.filter<on_add<int>>().size();
			require(e_list2.size()) == state.filter<int>().size();
			token = state.declare(
			  [size_1 = e_list1.size(), size_2 = e_list2.size()](psl::ecs::info_t& info,
																 pack<entity, const int, on_remove<int>> pack1,
																 pack<entity, const int, filter<int>> pack2) {
				  for(auto [e, i] : pack1)
				  {
					  require(e) == i;
				  }
				  require(pack1.get<entity>()[0]) == 0;
				  // if this shows 0, then the previous deleted components of tick #1 are still present
				  require(pack2.get<entity>()[0]) == 10;
				  for(auto [e, i] : pack2)
				  {
					  require(e) == i;
				  }
				  require(pack1.size()) == size_1;
				  require(pack2.size()) == size_2;
			  });

			// tick #2
			// we verify the elements of e_list1 are deleted and their data is intact
			// we verify the elements of e_list2 are added and their data is correct
			// we also remove all elements of e_list2
			state.tick(std::chrono::duration<float>(0.1f));
			state.revoke(token);

			// pre-tick #3
			// we verify that no int component is present anymore in the system aside from the previously removed ones
			require(e_list2.size()) == state.filter<on_remove<int>>().size();
			require(0) == state.filter<int>().size();
			token = state.declare([size_2 = e_list2.size()](psl::ecs::info_t& info,
															pack<entity, on_remove<int>> pack1,
															pack<entity, filter<int>> pack2) {
				require(pack1.size()) == size_2;
				require(pack2.size()) == 0;
			});

			// tick #3
			state.tick(std::chrono::duration<float>(0.1f));
			state.revoke(token);
			token = state.declare(
			  [](psl::ecs::info_t& info, pack<entity, on_remove<int>> pack1, pack<entity, filter<int>> pack2) {
				  require(pack1.size()) == 0;
				  require(pack2.size()) == 0;
			  });

			// tick #4
			state.tick(std::chrono::duration<float>(0.1f));

			// tick #5
			state.tick(std::chrono::duration<float>(0.1f));

			require(0) == state.filter<on_remove<int>>().size();
			require(0) == state.filter<int>().size();
		};
		
		section<"continuous removal from within systems">() = [&]() {
			auto e_list2 {state.create(40)};
			psl::array<int> values;
			values.resize(e_list2.size());
			std::iota(std::begin(values), std::end(values), 0);
			;
			state.add_components<int>(e_list2, values);
			auto expected = e_list2.size();

			state.declare([&expected](psl::ecs::info_t& info, pack<entity, int> pack) {
				require(pack.size()) == expected;
				psl::array<entity> entities;
				for(auto [e, i] : pack)
				{
					require(static_cast<int>(e)) == i;
					if(std::rand() % 2 == 0)
					{
						entities.emplace_back(e);
						--expected;
					}
				}
				info.command_buffer.remove_components<int>(entities);
			});

			while(expected > 0) state.tick(std::chrono::duration<float>(0.1f));
		};


		section<"continuous removal from external">() = [&]() {
			auto e_list2 {state.create(40)};
			psl::array<int> values;
			values.resize(e_list2.size());
			std::iota(std::begin(values), std::end(values), 0);
			state.add_components<int>(e_list2, values);
			auto expected = e_list2.size();

			state.declare([&expected](psl::ecs::info_t& info, pack<entity, int> pack) {
				require(pack.size()) == expected;
				for(auto [e, i] : pack)
				{
					require(static_cast<int>(e)) == i;
				}
			});

			while(expected > 0)
			{
				state.tick(std::chrono::duration<float>(0.1f));

				auto mid =
				  std::partition(std::begin(e_list2), std::end(e_list2), [](auto e) { return std::rand() % 2; });
				state.remove_components<int>(
				  psl::array_view<entity> {mid, static_cast<size_t>(std::distance(mid, std::end(e_list2)))});
				expected -= std::distance(mid, std::end(e_list2));
				e_list2.erase(mid, std::end(e_list2));
			}
		};

		section<"continuous addition from within systems">() = [&]() {
			auto e_list2 {state.create(40)};
			psl::array<int> values;
			values.resize(e_list2.size());
			std::iota(std::begin(values), std::end(values), 0);
			state.add_components<int>(e_list2, values);
			auto expected = e_list2.size();

			state.declare([&expected](psl::ecs::info_t& info, pack<entity, int> pack) {
				require(pack.size()) == expected;

				for(auto [e, i] : pack)
				{
					require(static_cast<int>(e)) == i;
				}
				auto new_count				= std::rand() % 20;
				psl::array<entity> entities = info.command_buffer.create(new_count);
				psl::array<int> values;
				values.resize(entities.size());
				std::iota(std::begin(values), std::end(values), static_cast<int>(expected));
				info.command_buffer.add_components<int>(entities, values);
				expected += new_count;
			});

			while(expected <= 1'000) state.tick(std::chrono::duration<float>(0.1f));
		};

		section<"continuous addition from external">() = [&]() {
			auto e_list2 {state.create(40)};
			{
				psl::array<int> values;
				values.resize(e_list2.size());
				std::iota(std::begin(values), std::end(values), 0);
				state.add_components<int>(e_list2, values);
			}
			auto expected = e_list2.size();

			state.declare([&expected](psl::ecs::info_t& info, pack<entity, int> pack) {
				require(pack.size()) == expected;

				for(auto [e, i] : pack)
				{
					require(static_cast<int>(e)) == i;
				}
			});

			while(expected <= 1'000)
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
		};

		section<"simple iterations test">() = [&]() {
			auto e_list1 {state.create(10)};
			auto e_list2 {state.create(40)};
			auto e_list3 {state.create(50)};
			state.add_components<float>(e_list1);
			state.add_components<int>(e_list1);
			auto system_id = state.declare(float_iteration_test);
			for(int i = 0; i < 10; ++i) state.tick(std::chrono::duration<float>(0.1f));

			auto entities = state.filter<int>();
			auto results  = state.view<int>();
			require(results.size()) == entities.size();
			require(results.size()) == e_list1.size();
			require(std::all_of(std::begin(results), std::end(results), [](const auto& res) { return res == 50; }));

			require(state.systems()) == 1;
			state.revoke(system_id);
			require(state.systems()) == 0;
			state.tick(std::chrono::duration<float>(0.1f));
			require(std::all_of(std::begin(results), std::end(results), [](const auto& res) { return res == 50; }));
		};
	};

	auto t5 = suite<"declaring system signatures", "ecs", "psl">() = []() {
		state_t state;
		state.declare(registration_test);
		object_test test;
		state.declare(&object_test::empty_system, &test);
		state.declare([](psl::ecs::info_t& info) {});
	};

	auto t7 = suite<"filtering over multiple frames", "ecs", "psl">() = []() {
		state_t state {1u};
		section<"regression 1">() = [&] {

			// issue: non-unique entry in filtering operation
			// order of operations:
			//  - add 1 entity -> becomes "modified entity" for filtering/ecs
			//  - when ticking system, modify the entity again
			//  - next tick when the results are collabed it will duplicate the entity
			//    in the filtering for system's filtering -> issue

			size_t count = 0;
			state.declare([&](info_t& info, pack<entity, wrapper_t<float>> pack) {
				if(pack.empty()) return;
				count += pack.size();
				info.command_buffer.add_components<wrapper_t<int>>(pack.get<entity>(), {0});
			});

			auto entities = state.create<wrapper_t<float>>(1);
			expect(state.filter<on_add<wrapper_t<float>>>().size()) == 1;
			state.tick(std::chrono::duration<float>(1.0f));
			expect(state.filter<wrapper_t<float>>().size()) == 1;
			expect(state.filter<on_add<wrapper_t<int>>>().size()) == 1;
			expect(count) == 1;
			state.tick(std::chrono::duration<float>(1.0f));
			expect(state.filter<wrapper_t<float>>().size()) == 1;
			expect(state.filter<wrapper_t<int>>().size()) == 1;
			expect(count) == 2;
		};
		section<"regression 2">() = [&] {

			// issue: incorrect filtering return for removed entities
			// operations:
			// - create 2 separate entities, one with a component and one without (component irrelevant)
			// - remove the component from the first batch of entities
			// - add the component to the second batch
			// - notice after ticking the present components in the system is incorrect
			// reason: filtering operation that was based on existing filters did not correctly
			//         use the already filtered entity list

			auto entities0 = state.create<wrapper_t<float>>(1);
			auto entities1 = state.create(1);
			state.remove_components<wrapper_t<float>>(entities0);
			expect(state.filter<wrapper_t<float>>().size()) == 0;
			expect(state.filter<on_remove<wrapper_t<float>>>().size()) == 1;
			state.add_components<wrapper_t<float>>(entities1);
			expect(state.filter<wrapper_t<float>>().size()) == 1;
			expect(state.filter<on_add<wrapper_t<float>>>().size()) == 1;
			expect(state.filter<on_remove<wrapper_t<float>>>().size()) == 1;
			state.declare([&](info_t& info, pack<entity, wrapper_t<float>> pack) { expect(pack.size()) == 1; 
				expect(pack.get<entity>()[0]) == 1;
			});
			state.tick(std::chrono::duration<float>(1.0f));
			expect(state.filter<wrapper_t<float>>().size()) == 1;
			expect(state.filter<on_remove<wrapper_t<float>>>().size()) == 0;
			expect(state.filter<on_add<wrapper_t<float>>>().size()) == 0;
		};
	};
}	 // namespace