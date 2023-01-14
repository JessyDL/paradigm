#include "ecs.hpp"
#include "psl/ecs/on_condition.hpp"
#include "psl/ecs/order_by.hpp"
#include "psl/ecs/state.hpp"
#include <random>

#include "psl/serialization/decoder.hpp"
#include "psl/serialization/encoder.hpp"

using namespace psl::ecs;
using namespace tests::ecs;

void registration_test(psl::ecs::info_t& info) {}


namespace tests::ecs {
template <IsPolicy Policy, IsAccessType Access>
void float_iteration_test(psl::ecs::info_t& info, psl::ecs::pack_t<Policy, Access, /*const*/ float, int> pack) {
	for(auto [fl, i] : pack) {
		i += 5;
	}
};

struct object_test {
	void empty_system(psl::ecs::info_t& info) {};
};
}	 // namespace tests::ecs

template <typename T>
auto to_num_string(T i) {
	static_assert(std::is_arithmetic_v<T>);
	auto conversion = std::to_string(i);
	auto it			= std::end(conversion);
	while(std::distance(std::begin(conversion), it) > 3) {
		it = std::prev(it, 3);
		conversion.insert(it, '.');
	}
	return conversion;
}

struct foo_renamed {};

namespace psl::ecs {
template <>
struct component_traits<foo_renamed> {
	static constexpr bool serializable {true};
	static constexpr auto name = "SOMEOVERRIDE";
};
}	 // namespace psl::ecs

#include <litmus/expect.hpp>
#include <litmus/section.hpp>
#include <litmus/suite.hpp>

using namespace litmus;

namespace {
struct position {
	size_t x;
	size_t y;
};

template <typename T>
struct complex_wrapper {
	complex_wrapper() = default;
	complex_wrapper(T val) : val(val) {}
	complex_wrapper(auto val) : val(static_cast<T>(val)) {}

	operator const T&() const noexcept { return val; }
	operator T&() noexcept { return val; }

	complex_wrapper& operator=(const T& rhs) {
		if(this != &rhs) {
			val = rhs;
		}
		return *this;
	}
	T val {};
};

struct complex_wrapper_float : public complex_wrapper<float> {
	using complex_wrapper<float>::complex_wrapper;
};

struct complex_wrapper_int : public complex_wrapper<int> {
	using complex_wrapper<int>::complex_wrapper;
};

struct flag_type {};

// components do not support templated typenames
using float_tpack  = tpack<float, complex_wrapper_float>;
using int_tpack	   = tpack<int, complex_wrapper_int>;
using policy_tpack = tpack<psl::ecs::partial_t, psl::ecs::full_t>;
using access_tpack = tpack<psl::ecs::direct_t, psl::ecs::indirect_t>;

auto t0 = suite<"component_info", "ecs", "psl">().templates<float_tpack>() = []<typename type>() {
	section<"non-empty component_info_typed">() = [&]() {
		auto cInfoPtr {details::instantiate_component_container<type>()};
		auto& cInfo = *details::cast_component_container<type>(cInfoPtr.get());

		section<"additions">() = [&]() {
			psl::array<entity> entities;
			entities.resize(100);
			std::iota(std::begin(entities), std::end(entities), entity {0});
			cInfo.add(entities);
			require(cInfo.size()) == entities.size();
			require(cInfo.added_entities().size()) == entities.size();
			std::for_each(std::begin(entities), std::end(entities), [&cInfo](entity e) { cInfo.set(e, type(e)); });
			std::for_each(std::begin(entities), std::end(entities), [&cInfo](entity e) {
				require(cInfo.has_component(e));
				require(cInfo.has_added(e));
				require(cInfo.entity_data().template at<type>(e)) == type(e);
			});

			section<"removals">() = [&]() {
				std::random_device rd;
				std::mt19937 g(rd());
				std::shuffle(std::begin(entities), std::end(entities), g);

				auto count = entities.size() / 10;
				for(entity c = 0; c < 10; ++c) {
					for(auto i = 0; i < count; ++i) {
						auto index = c * 10 + i;
						cInfo.destroy(entities[index]);
					}

					for(entity i = 0; i < static_cast<entity>(entities.size()); ++i) {
						if(i < (c + 1) * 10) {
							auto index = entities[i];
							require(!cInfo.has_component(index));
							require(cInfo.has_removed(index));
							require(cInfo.entity_data().template at<type>(index, details::stage_range_t::REMOVED)) ==
							  type(index);
						} else {
							auto index = entities[i];
							require(cInfo.has_component(index));
							require(cInfo.entity_data().template at<type>(index)) == type(index);
						}
					}
				}
			};
		};


		// section<"additions && removals">() = [&](){};
		section<"remap">() = [&]() {
			auto cInfo2Ptr {details::instantiate_component_container<type>()};
			auto& cInfo2 = *details::cast_component_container<type>(cInfo2Ptr.get());
			psl::array<entity> entities;
			entities.resize(100);
			std::iota(std::begin(entities), std::end(entities), entity {0});
			cInfo.add(entities);
			cInfo2.add(entities);

			std::for_each(std::begin(entities), std::end(entities), [&cInfo](entity e) { cInfo.set(e, type(e)); });

			std::for_each(
			  std::begin(entities),
			  std::end(entities),
			  [&cInfo2, offset = static_cast<entity>(cInfo.size())](entity e) { cInfo2.set(e, type(e + offset)); });

			psl::sparse_array<entity> remap;
			std::for_each(std::begin(entities),
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
								  require(cInfo.entity_data().template operator[]<type>(e)) == type(e);
							  });
			};
		};
	};
	// section<"empty component_info_typed">() = [&]() {};
};

auto t1 = suite<"component_key must be unique", "ecs", "psl">().templates<float_tpack>() = []<typename type>() {
	using namespace psl::ecs::details;
	auto fl_id	= component_key_t::generate<type>();
	auto int_id = component_key_t::generate<int>();
	require(fl_id) != int_id;

	auto cfl_id	 = component_key_t::generate<const type>();
	auto cint_id = component_key_t::generate<const int>();
	require(cfl_id) != cint_id;
	require(cfl_id) == fl_id;
	require(cint_id) == int_id;

	constexpr auto cxfl_id	= component_key_t::generate<type>();
	constexpr auto cxint_id = component_key_t::generate<int>();

	require(cxfl_id) == fl_id;
	require(cxint_id) == int_id;
};

auto t2 = suite<"filtering", "ecs", "psl">()
			.templates<tpack<float, complex_wrapper_float, flag_type>, policy_tpack, access_tpack>() =
  []<typename type, typename policy, typename access>() {
	  state_t state;
	  auto e_list1 {state.create(100)};
	  auto e_list2 {state.create(400)};
	  auto e_list3 {state.create(500)};


	  section<"only the first 100 are given all components">() = [&]() {
		  state.add_components<type>(e_list1);
		  state.add_components<size_t>(e_list1);
		  state.add_components<int>(e_list1);
		  state.add_components<char>(e_list1);
		  state.add_components<std::byte>(e_list1);

		  auto f = state.filter<type, size_t, int, char, std::byte>();
		  require(f.size()) == e_list1.size();
		  require(std::equal(std::begin(f), std::end(f), std::begin(e_list1)));
		  require(state.filter<type>().size()) == state.filter<on_add<type>>().size();
		  require(state.filter<type>().size()) == state.filter<on_combine<type, size_t>>().size();
		  require(state.filter<on_remove<type>>().size()) == state.filter<on_break<type, size_t>>().size();
	  };

	  section<"500 are given two component types and 100 are given 3 component types where 2 overlap">() = [&]() {
		  state.add_components<int>(e_list1);
		  state.add_components<char>(e_list1);
		  state.add_components<size_t>(e_list1);

		  state.add_components<char>(e_list3);
		  state.add_components<size_t>(e_list3);


		  auto f = state.filter<type, size_t, int, char, std::byte>();
		  require(f.size()) == 0;

		  f = state.filter<size_t, int, char>();
		  require(f.size()) == e_list1.size();
		  require(std::equal(std::begin(f), std::end(f), std::begin(e_list1)));

		  f								  = state.filter<size_t, char>();
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
		  state.create<type, size_t>(500);
		  state.destroy(1200);
		  auto entities = state.create<type, size_t>(3);

		  require(entities.size()) == 3;
		  require(entities[0]) == 1500;
		  require(entities[1]) == 1501;
		  require(entities[2]) == 1502;
		  require(state.filter<type>().size()) == 502;	  // 500 + 3 - 1
		  require(state.filter<type>().size()) == state.filter<on_add<type>>().size();
		  require(state.filter<type>().size()) == state.filter<on_combine<type, size_t>>().size();
		  require(state.filter<on_remove<type>>().size()) == 1;	   // we deleted entity 1200
		  require(state.filter<on_remove<type>>().size()) == state.filter<on_break<type, size_t>>().size();
	  };

	  section<"on_condition">() = [&]() {
		  state.add_components<position>(e_list1, position {100, 500});
		  state.add_components<position>(e_list2, position {10, 30});

		  auto on_condition_func = [](const position& pos) { return (pos.x + pos.y) > 100; };

		  state.declare([elist1_size = e_list1.size()](
						  info_t& info,
			  pack_t<psl::ecs::full_t, access, position, on_condition<decltype(on_condition_func), position>> pack) {
			  expect(pack.size()) == elist1_size;
		  });

		  state.declare([total_size = e_list1.size() + e_list2.size()](info_t& info, pack_t<psl::ecs::full_t, access, position> pack) {
			  expect(pack.size()) == total_size;
		  });

		  state.tick(std::chrono::duration<float>(1.0f));
	  };

	  section<"order_by">() = [&]() {
		  state.add_components(e_list1, [](position& pos) {
			  pos.x = std::rand() % 10;
			  pos.y = std::rand() % 10;
		  });

		  auto order_by_func = [](const position& lhs, const position& rhs) {
			  return (lhs.x == rhs.x) ? lhs.y < rhs.y : lhs.x < rhs.x;
		  };

		  state.declare(
			[](info_t& info, pack_t<policy, access, position, order_by<decltype(order_by_func), position>> pack) {
				auto last_x = std::numeric_limits<decltype(position::x)>::min();
				auto last_y = std::numeric_limits<decltype(position::y)>::min();

				for(auto [position] : pack) {
					expect(last_x) <= position.x;

					if(last_x == position.x)
						expect(last_y) <= position.y;

					last_x = position.x;
					last_y = position.y;
				}
			});

		  state.tick(std::chrono::duration<float>(1.0f));
	  };
  };

auto t3 = suite<"initializing components", "ecs", "psl">().templates<float_tpack>() = []<typename type>() {
	state_t state;

	size_t count {0};
	state.create(
	  50,
	  [&count](position& i) {
		  i = {++count, 0};
	  },
	  type {5.0f},
	  psl::ecs::empty<size_t>());

	require(state.view<position>().size()) == 50;
	require(state.view<type>().size()) == 50;
	require(state.view<size_t>().size()) == 50;

	count = {0};
	size_t check {0};
	size_t check_view {0};
	for(const auto& i : state.view<position>()) {
		++count;
		check += count;
		check_view += i.x;
	}

	require(check_view) == check;
};

auto t4 = suite<"systems", "ecs", "psl">().templates<int_tpack, policy_tpack, access_tpack>() =
  []<typename type, typename policy, typename access>() {
	  state_t state;

	  auto group = details::make_filter_group(psl::type_pack_t<pack_direct_full_t<entity, type>> {});


	  section<"lifetime test">() = [&]() {
		  auto e_list1 {state.create(10)};
		  auto e_list2 {state.create(40)};
		  auto e_list3 {state.create(50)};
		  // pre-tick #1
		  // we add int components to all elements in e_list1, by giving them an incrementing value
		  // thanks to these being the first entities, they overlap with their ID
		  size_t incrementer = 0;
		  state.add_components(e_list1, [&incrementer](type& target) { target = type(incrementer++); });

		  state.declare([](psl::ecs::info_t& info, pack_direct_full_t<entity, filter<type>> pack) {
			  info.command_buffer.destroy(pack.template get<entity>());
		  });
		  auto token = state.declare(
			[size_1 = e_list1.size()](psl::ecs::info_t& info, pack_direct_full_t<entity, const type> pack1) {
				for(auto [e, i] : pack1) {
					require(e) == i;
				}
				require(pack1.size()) == size_1;
			});

		  require(e_list1.size()) == state.filter<on_add<type>>().size();
		  require(e_list1.size()) == state.filter<type>().size();

		  // tick #1
		  // here we verify the resources of e_list1 are all present, and their values accurate
		  // followed by deleting them all.
		  state.tick(std::chrono::duration<float>(0.1f));
		  {
			  for(auto e : e_list1) {
				  auto val = state.template get<type>(e);
				  require(e) == val;
			  }
		  }
		  state.revoke(token);

		  // pre-tick #2
		  // we add int components to all elements in e_list2, by giving them an incrementing value with offset of
		  // elist1.size(), thanks to these being the first entities, they overlap with their ID
		  require(e_list1.size()) == state.filter<on_remove<type>>().size();
		  require(0) == state.filter<type>().size();
		  require(0) == state.filter<on_add<type>>().size();
		  incrementer = e_list1.size();
		  state.add_components(e_list2, [&incrementer](type& target) { target = type(incrementer++); });
		  require(e_list2.size()) == state.filter<on_add<type>>().size();
		  require(e_list2.size()) == state.filter<type>().size();
		  token = state.declare([size_1 = e_list1.size(), size_2 = e_list2.size()](
								  psl::ecs::info_t& info,
								  pack_direct_full_t<entity, const type, on_remove<type>> pack1,
								  pack_direct_full_t<entity, const type, filter<type>> pack2) {
			  for(auto [e, i] : pack1) {
				  require(e) == i;
			  }
			  require(pack1.template get<entity>()[0]) == 0;
			  // if this shows 0, then the previous deleted components of tick #1 are still present
			  require(pack2.template get<entity>()[0]) == 10;
			  for(auto [e, i] : pack2) {
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
		  require(e_list2.size()) == state.filter<on_remove<type>>().size();
		  require(0) == state.filter<type>().size();
		  token = state.declare([size_2 = e_list2.size()](psl::ecs::info_t& info,
														  pack_t<psl::ecs::full_t, access, type, on_remove<type>> pack1,
														  pack_t<psl::ecs::full_t, access, type, filter<type>> pack2) {
			  require(pack1.size()) == size_2;
			  require(pack2.size()) == 0;
		  });

		  // tick #3
		  state.tick(std::chrono::duration<float>(0.1f));
		  state.revoke(token);
		  token = state.declare([](psl::ecs::info_t& info,
								   pack_t<policy, access, on_remove<type>> pack1,
								   pack_t<policy, access, filter<type>> pack2) {
			  require(pack1.size()) == 0;
			  require(pack2.size()) == 0;
		  });

		  // tick #4
		  state.tick(std::chrono::duration<float>(0.1f));

		  // tick #5
		  state.tick(std::chrono::duration<float>(0.1f));

		  require(0) == state.filter<on_remove<type>>().size();
		  require(0) == state.filter<type>().size();
	  };

	  section<"continuous removal from within systems">() = [&]() {
		  auto e_list2 {state.create(40)};
		  psl::array<type> values;
		  values.resize(e_list2.size());
		  std::iota(std::begin(values), std::end(values), 0);
		  ;
		  state.add_components<type>(e_list2, values);
		  auto expected = e_list2.size();

		  state.declare([&expected](psl::ecs::info_t& info, pack_direct_full_t<entity, type> pack) {
			  require(pack.size()) == expected;
			  psl::array<entity> entities;
			  for(auto [e, i] : pack) {
				  require(type(e)) == i;
				  if(std::rand() % 2 == 0) {
					  entities.emplace_back(e);
					  --expected;
				  }
			  }
			  info.command_buffer.remove_components<type>(entities);
		  });

		  while(expected > 0) state.tick(std::chrono::duration<float>(0.1f));
	  };


	  section<"continuous removal from external">() = [&]() {
		  auto e_list2 {state.create(40)};
		  psl::array<type> values;
		  values.resize(e_list2.size());
		  std::iota(std::begin(values), std::end(values), 0);
		  state.add_components<type>(e_list2, values);
		  auto expected = e_list2.size();

		  state.declare([&expected](psl::ecs::info_t& info, pack_direct_full_t<entity, type> pack) {
			  require(pack.size()) == expected;
			  for(auto [e, i] : pack) {
				  require(type(e)) == i;
			  }
		  });

		  while(expected > 0) {
			  state.tick(std::chrono::duration<float>(0.1f));

			  auto mid = std::partition(std::begin(e_list2), std::end(e_list2), [](auto e) { return std::rand() % 2; });
			  state.remove_components<type>(
				psl::array_view<entity> {mid, static_cast<size_t>(std::distance(mid, std::end(e_list2)))});
			  expected -= std::distance(mid, std::end(e_list2));
			  e_list2.erase(mid, std::end(e_list2));
		  }
	  };

	  section<"continuous addition from within systems">() = [&]() {
		  auto e_list2 {state.create(40)};
		  psl::array<type> values;
		  values.resize(e_list2.size());
		  std::iota(std::begin(values), std::end(values), 0);
		  state.add_components<type>(e_list2, values);
		  auto expected = e_list2.size();

		  state.declare([&expected](psl::ecs::info_t& info, pack_direct_full_t<entity, type> pack) {
			  require(pack.size()) == expected;

			  for(auto [e, i] : pack) {
				  require(type(e)) == i;
			  }
			  auto new_count			  = std::rand() % 20;
			  psl::array<entity> entities = info.command_buffer.create(new_count);
			  psl::array<type> values;
			  values.resize(entities.size());
			  std::iota(std::begin(values), std::end(values), type(expected));
			  info.command_buffer.add_components<type>(entities, values);
			  expected += new_count;
		  });

		  while(expected <= 1'000) state.tick(std::chrono::duration<float>(0.1f));
	  };

	  section<"continuous addition from external">() = [&]() {
		  auto e_list2 {state.create(40)};
		  {
			  psl::array<type> values;
			  values.resize(e_list2.size());
			  std::iota(std::begin(values), std::end(values), 0);
			  state.add_components<type>(e_list2, values);
		  }
		  auto expected = e_list2.size();

		  state.declare([&expected](psl::ecs::info_t& info, pack_direct_full_t<entity, type> pack) {
			  require(pack.size()) == expected;

			  for(auto [e, i] : pack) {
				  require(type(e)) == i;
			  }
		  });

		  while(expected <= 1'000) {
			  state.tick(std::chrono::duration<float>(0.1f));

			  auto new_count			  = std::rand() % 20;
			  psl::array<entity> entities = state.create(new_count);
			  psl::array<type> values;
			  values.resize(entities.size());
			  std::iota(std::begin(values), std::end(values), type(expected));
			  state.add_components<type>(entities, values);
			  expected += new_count;
		  }
	  };

	  section<"simple iterations test">() = [&]() {
		  auto e_list1 {state.create(10)};
		  auto e_list2 {state.create(40)};
		  auto e_list3 {state.create(50)};
		  state.add_components<float>(e_list1);
		  state.add_components<type>(e_list1);
		  auto system_id = state.declare(float_iteration_test<policy, access>);
		  for(int i = 0; i < 10; ++i) state.tick(std::chrono::duration<float>(0.1f));

		  auto entities = state.filter<type>();
		  auto results	= state.view<type>();
		  require(results.size()) == entities.size();
		  require(results.size()) == e_list1.size();
		  require(std::all_of(std::begin(results), std::end(results), [](const auto& res) { return res == type(50); }));

		  require(state.systems()) == 1;
		  state.revoke(system_id);
		  require(state.systems()) == 0;
		  state.tick(std::chrono::duration<float>(0.1f));
		  require(std::all_of(std::begin(results), std::end(results), [](const auto& res) { return res == type(50); }));
	  };
  };

auto t5 = suite<"declaring system signatures", "ecs", "psl", "regression">() = []() {
	state_t state;
	state.declare(registration_test);
	object_test test;
	state.declare(&object_test::empty_system, &test);
	state.declare([](psl::ecs::info_t& info) {});
};

auto t7 =
  suite<"filtering over multiple frames", "ecs", "psl", "regression">().templates<float_tpack, policy_tpack, access_tpack>() = []<typename type, typename policy, typename access>() {
	  state_t state {1u};
	  section<"regression 1">() = [&] {
		  // issue: non-unique entry in filtering operation
		  // order of operations:
		  //  - add 1 entity -> becomes "modified entity" for filtering/ecs
		  //  - when ticking system, modify the entity again
		  //  - next tick when the results are collabed it will duplicate the entity
		  //    in the filtering for system's filtering -> issue

		  size_t count = 0;
		  state.declare([&](info_t& info, pack_direct_full_t<entity, type> pack) {
			  if(pack.empty())
				  return;
			  count += pack.size();
			  info.command_buffer.add_components<int>(pack.template get<entity>(), {0});
		  });

		  auto entities = state.create<type>(1);
		  expect(state.filter<on_add<type>>().size()) == 1;
		  state.tick(std::chrono::duration<float>(1.0f));
		  expect(state.filter<type>().size()) == 1;
		  expect(state.filter<on_add<int>>().size()) == 1;
		  expect(count) == 1;
		  state.tick(std::chrono::duration<float>(1.0f));
		  expect(state.filter<type>().size()) == 1;
		  expect(state.filter<int>().size()) == 1;
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

		  auto entities0 = state.create<type>(1);
		  auto entities1 = state.create(1);
		  state.remove_components<type>(entities0);
		  expect(state.filter<type>().size()) == 0;
		  expect(state.filter<on_remove<type>>().size()) == 1;
		  state.add_components<type>(entities1);
		  expect(state.filter<type>().size()) == 1;
		  expect(state.filter<on_add<type>>().size()) == 1;
		  expect(state.filter<on_remove<type>>().size()) == 1;
		  state.declare([&](info_t& info, pack_direct_full_t<entity, type> pack) {
			  expect(pack.size()) == 1;
			  expect(pack.template get<entity>()[0]) == 1;
		  });
		  state.tick(std::chrono::duration<float>(1.0f));
		  expect(state.filter<type>().size()) == 1;
		  expect(state.filter<on_remove<type>>().size()) == 0;
		  expect(state.filter<on_add<type>>().size()) == 0;
	  };
  };


auto t8 = suite<"component_key name matches expected", "ecs", "psl">() = []() {
	using namespace psl::ecs::details;
	using namespace std::string_view_literals;
	auto fl_id	= component_key_t::generate<float>();
	auto int_id = component_key_t::generate<int>();
	require(fl_id.name()) == "float"sv;
	require(int_id.name()) == "int"sv;

	auto cfl_id	 = component_key_t::generate<const float>();
	auto cint_id = component_key_t::generate<const int>();
	require(cfl_id.name()) == "float"sv;
	require(cint_id.name()) == "int"sv;

	constexpr auto cxfl_id	= component_key_t::generate<float>();
	constexpr auto cxint_id = component_key_t::generate<int>();

	require(cxfl_id.name()) == "float"sv;
	require(cxint_id.name()) == "int"sv;

	require(component_key_t::generate<foo_renamed>().name()) == "SOMEOVERRIDE"sv;
};

auto t9 = suite<"ecs state serialization", "ecs", "psl">() = []() {
	psl::ecs::state_t state_a {}, state_b {};
	int counter {0};
	state_a.create(200, [&counter](int& value) { value = counter++; });
	state_a.override_serialization<int>(true);

	psl::serialization::serializer serializer {};

	psl::format::container container_a {};
	serializer.serialize<psl::serialization::encode_to_format>(state_a, container_a);
	serializer.deserialize<psl::serialization::decode_from_format>(state_b, container_a);

	require(state_a.size<int>()) == state_b.size<int>();
	require(state_a.size<int>()) == 200;

	auto entities	  = state_a.entities<int>();
	auto components_a = state_a.get_component<int>(entities);
	auto components_b = state_b.get_component<int>(entities);

	for(size_t i = 0; i < components_a.size(); ++i) {
		require(components_a[i]) == components_b[i];
		require(entities[i]) == components_a[i];
	}

	psl::format::container container_b {};
	serializer.serialize<psl::serialization::encode_to_format>(state_b, container_b);

	require(container_b.to_string()) == container_a.to_string();
};

struct foo {
	static constexpr auto prototype() -> foo { return foo {10}; }
	int value;
};

auto t10 = suite<"ecs prototype support", "ecs", "psl">() = []() {
	psl::ecs::state_t state {};
	auto entity = state.create<foo>(1);
	require(state.get<foo>(entity[0]).value) == 10;
};
}	 // namespace
