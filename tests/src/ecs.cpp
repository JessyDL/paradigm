#include "ecs.hpp"
#include "psl/ecs/on_condition.hpp"
#include "psl/ecs/order_by.hpp"
#include "psl/ecs/state.hpp"
#include <mutex>
#include <random>

#include "psl/serialization/decoder.hpp"
#include "psl/serialization/encoder.hpp"

using namespace psl::ecs;
using namespace tests::ecs;

void registration_test(psl::ecs::info_t& info) {}

namespace std {
std::string to_string(entity_t entity) {
	return entity.valid() ? std::string("Entity {") + std::to_string(entity.value()) + "}"
						  : std::string("Entity { INVALID }");
}
}	 // namespace std


namespace tests::ecs {
template <IsPolicy Policy, IsAccessType Access, typename First, typename Second>
void float_iteration_test(psl::ecs::info_t& info, psl::ecs::pack_t<Policy, Access, const First, Second> pack) {
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
struct component_trait_serializable_t<foo_renamed> {
	static constexpr bool serializable {true};
};
template <>
struct component_trait_name_t<foo_renamed> {
	static constexpr auto name = "SOMEOVERRIDE";
};
}	 // namespace psl::ecs


struct updated_component {
	bool some_new_value;
	float other;
	int value;
};
template <>
struct psl::ecs::component_trait_version_t<updated_component> {
	static constexpr size_t version = 1;
};

template <>
struct psl::ecs::component_updater_t<updated_component> {
	updated_component operator()(size_t version, void* data) {
		updated_component res {};
		switch(version) {
		case 0: {
			// here we have the layout of how the component used to look like.
			struct updated_component_v0 {
				int value;
			};

			res.value = reinterpret_cast<updated_component_v0*>(data)->value;
		} break;
		default:
			throw std::runtime_error("invalid version");
		}
		return res;
	}
};

struct foo_restricted {
	int value1;
	float value2;
	bool value3;
};

namespace psl::ecs {
template <>
struct component_trait_mutability_t<foo_restricted> {
	static constexpr component_mutability_behaviour_t mutability = component_mutability_behaviour_t::restricted;
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

	operator const T&() const noexcept {
		return val;
	}
	operator T&() noexcept {
		return val;
	}

	complex_wrapper& operator+=(const T& rhs) {
		val += rhs;
		return *this;
	}

	complex_wrapper& operator=(const T& rhs) {
		if(this != &rhs) {
			val = rhs;
		}
		return *this;
	}
	T val {};
};

psl::array<entity_t> make_entities_range(size_t count, size_t offset = 0) {
	psl::array<entity_t> entities;
	entities.reserve(count);
	for(size_t i = 0; i < count; ++i) {
		entities.emplace_back(details::make_entity(static_cast<entity_t::size_type>(i + offset)));
	}
	return entities;
}

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
			psl::array<entity_t> entities {make_entities_range(100)};
			cInfo.add(entities);
			require(cInfo.size()) == entities.size();
			require(cInfo.added_entities().size()) == entities.size();
			std::for_each(std::begin(entities), std::end(entities), [&cInfo](entity_t e) {
				cInfo.set(e, type(static_cast<entity_t::size_type>(e)));
			});
			std::for_each(std::begin(entities), std::end(entities), [&cInfo](entity_t e) {
				require(cInfo.has_component(e));
				require(cInfo.has_added(e));
				require(cInfo.entity_data().template at<type>(static_cast<entity_t::size_type>(e))) ==
				  type(static_cast<entity_t::size_type>(e));
			});

			section<"removals">() = [&]() {
				std::random_device rd;
				std::mt19937 g(rd());
				std::shuffle(std::begin(entities), std::end(entities), g);

				auto count = entities.size() / 10;
				for(entity_t::size_type c = 0; c < 10; ++c) {
					for(entity_t::size_type i = 0; i < count; ++i) {
						auto index = c * 10 + i;
						cInfo.destroy(entities[index]);
					}

					for(entity_t::size_type i = 0; i < static_cast<entity_t::size_type>(entities.size()); ++i) {
						if(i < (c + 1) * 10) {
							auto index = entities[i];
							require(!cInfo.has_component(index));
							require(cInfo.has_removed(index));
							require(cInfo.entity_data().template at<type>(static_cast<entity_t::size_type>(index),
																		  details::stage_range_t::REMOVED)) ==
							  type(static_cast<entity_t::size_type>(index));
						} else {
							auto index = entities[i];
							require(cInfo.has_component(index));
							require(cInfo.entity_data().template at<type>(static_cast<entity_t::size_type>(index))) ==
							  type(static_cast<entity_t::size_type>(index));
						}
					}
				}
			};
		};


		// section<"additions && removals">() = [&](){};
		section<"remap">() = [&]() {
			auto cInfo2Ptr {details::instantiate_component_container<type>()};
			auto& cInfo2				  = *details::cast_component_container<type>(cInfo2Ptr.get());
			psl::array<entity_t> entities = make_entities_range(100);
			cInfo.add(entities);
			cInfo2.add(entities);

			std::for_each(std::begin(entities), std::end(entities), [&cInfo](entity_t e) {
				cInfo.set(e, type(static_cast<entity_t::size_type>(e)));
			});

			std::for_each(std::begin(entities),
						  std::end(entities),
						  [&cInfo2, offset = static_cast<entity_t::size_type>(cInfo.size())](entity_t e) {
							  cInfo2.set(e, type(static_cast<entity_t::size_type>(e) + offset));
						  });

			psl::sparse_array<entity_t::size_type, entity_t::size_type> remap;
			std::for_each(std::begin(entities),
						  std::end(entities),
						  [&remap, offset = static_cast<entity_t::size_type>(cInfo.size())](entity_t e) {
							  remap[static_cast<entity_t::size_type>(e)] = static_cast<entity_t::size_type>(e) + offset;
						  });
			cInfo2.remap(remap, [offset = static_cast<entity_t::size_type>(cInfo.size())](entity_t e) {
				return static_cast<entity_t::size_type>(e) < offset;
			});
			std::for_each(std::begin(cInfo2.entities()),
						  std::end(cInfo2.entities()),
						  [offset = static_cast<entity_t::size_type>(cInfo.size())](entity_t e) {
							  require(static_cast<entity_t::size_type>(e)) >= offset;
						  });
			section<"merge">() = [&]() {
				auto orig_size = cInfo.size();
				cInfo.merge(cInfo2);
				require(cInfo.size()) == orig_size + cInfo2.size();
				std::for_each(std::begin(cInfo.entities()),
							  std::end(cInfo.entities()),
							  [&cInfo, offset = static_cast<entity_t::size_type>(cInfo.size())](entity_t e) {
								  require(static_cast<entity_t::size_type>(e)) <= offset;
								  require(cInfo.entity_data().template operator[]<type>(
									static_cast<entity_t::size_type>(e))) == type(static_cast<entity_t::size_type>(e));
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
	  state_t state {};
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

		  f									= state.filter<size_t, char>();
		  std::vector<entity_t> combination = e_list1;
		  combination.reserve(e_list1.size() + e_list3.size());
		  combination.insert(std::end(combination), std::begin(e_list3), std::end(e_list3));
		  require(std::equal(std::begin(f), std::end(f), std::begin(combination)));
		  require(f.size()) == combination.size();
		  f = state.filter<char>();
		  require(std::equal(std::begin(f), std::end(f), std::begin(combination)));
		  require(f.size()) == combination.size();
	  };

	  section<"filtering components that are non-contiguous">() = [&]() {
		  auto last_created_entities = state.create<type, size_t>(static_cast<entity_t::size_type>(500));
		  state.destroy(last_created_entities.back());
		  auto entities = state.create<type, size_t>(static_cast<entity_t::size_type>(3));

		  require(entities.size()) == 3;
		  require(static_cast<entity_t::size_type>(entities[0])) == 1500;
		  require(static_cast<entity_t::size_type>(entities[1])) == 1501;
		  require(static_cast<entity_t::size_type>(entities[2])) == 1502;
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

		  size_t total_1 {0};
		  size_t total_2 {0};
		  std::mutex lock_1 {};
		  std::mutex lock_2 {};

		  state.declare([&total_1, &lock_1](
						  info_t& info,
						  pack_t<policy, access, position, on_condition<decltype(on_condition_func), position>> pack) {
			  std::lock_guard<std::mutex> guard(lock_1);
			  total_1 += pack.size();
		  });

		  state.declare([&total_2, &lock_2](info_t& info, pack_t<policy, access, position> pack) {
			  std::lock_guard<std::mutex> guard(lock_2);
			  total_2 += pack.size();
		  });

		  state.tick(std::chrono::duration<float>(1.0f));
		  expect(total_1) == e_list1.size();
		  expect(total_2) == (e_list1.size() + e_list2.size());
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
	  static_cast<entity_t::size_type>(50),
	  [&count](position& i) { i = {++count, 0}; },
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

	  section<"transient_systems">() = [&]() {
		  // transient systems are only executed once and removed after the tick is done.
		  bool has_triggered {false};
		  state.declare(transient_system_tag,
						[&has_triggered](psl::ecs::info_t& info, psl::ecs::pack_indirect_full_t<entity_t> pack) {
							require(pack.size()) == 0;			// no entities should be present in this pack
							require(has_triggered) == false;	// this should not have been triggered yet
							has_triggered =
							  true;	   // we set this to true to indicate that the system has been triggered
						});
		  state.tick(std::chrono::duration<float>(1.0f));
		  require(has_triggered) == true;	 // after the tick, the system should have been triggered
		  state.tick(std::chrono::duration<float>(1.0f));
	  };

	  section<"lifetime test">() = [&]() {
		  auto e_list1 {state.create(static_cast<entity_t::size_type>(10))};
		  auto e_list2 {state.create(static_cast<entity_t::size_type>(40))};
		  auto e_list3 {state.create(static_cast<entity_t::size_type>(50))};
		  // pre-tick #1
		  // we add int components to all elements in e_list1, by giving them an incrementing value
		  // thanks to these being the first entities, they overlap with their ID
		  size_t incrementer = 0;
		  state.add_components(e_list1, [&incrementer](type& target) { target = type(incrementer++); });

		  state.declare([](psl::ecs::info_t& info, pack_t<policy, access, entity_t, filter<type>> pack) {
			  info.command_buffer.destroy(pack.template get<entity_t>());
		  });

		  size_t total_pack1 {0};
		  size_t total_pack2 {0};
		  size_t total_pack3 {0};
		  std::mutex lock {};

		  auto token = state.declare(
			[&lock, &total_pack1](psl::ecs::info_t& info, pack_t<policy, access, entity_t, const type> pack1) {
				for(auto [e, i] : pack1) {
					require(static_cast<entity_t::size_type>(e)) == i;
				}

				std::lock_guard<std::mutex> guard(lock);
				total_pack1 += pack1.size();
			});

		  require(e_list1.size()) == state.filter<on_add<type>>().size();
		  require(e_list1.size()) == state.filter<type>().size();

		  // tick #1
		  // here we verify the resources of e_list1 are all present, and their values accurate
		  // followed by deleting them all.
		  state.tick(std::chrono::duration<float>(0.1f));

		  expect(total_pack1) == e_list1.size();
		  total_pack1 = 0;
		  {
			  for(auto e : e_list1) {
				  auto val = state.template get<type>(e);
				  require(static_cast<entity_t::size_type>(e)) == val;
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

		  token = state.declare(
			[&lock, &total_pack1, &total_pack2](psl::ecs::info_t& info,
												pack_t<policy, access, entity_t, const type, on_remove<type>> pack1,
												pack_t<policy, access, entity_t, const type, filter<type>> pack2) {
				for(auto [e, i] : pack1) {
					require(static_cast<entity_t::size_type>(e)) == i;
				}
				require(static_cast<entity_t::size_type>(pack1.template get<entity_t>()[0])) == 0;
				// if this shows 0, then the previous deleted components of tick #1 are still present
				require(static_cast<entity_t::size_type>(pack2.template get<entity_t>()[0])) == 10;
				for(auto [e, i] : pack2) {
					require(static_cast<entity_t::size_type>(e)) == i;
				}
				std::lock_guard<std::mutex> guard(lock);
				total_pack1 += pack1.size();
				total_pack2 += pack2.size();
			});

		  // tick #2
		  // we verify the elements of e_list1 are deleted and their data is intact
		  // we verify the elements of e_list2 are added and their data is correct
		  // we also remove all elements of e_list2
		  state.tick(std::chrono::duration<float>(0.1f));
		  expect(total_pack1) == e_list1.size();
		  expect(total_pack2) == e_list2.size();
		  total_pack1 = 0;
		  total_pack2 = 0;
		  state.revoke(token);

		  // pre-tick #3
		  // we verify that no int component is present anymore in the system aside from the previously removed ones
		  require(e_list2.size()) == state.filter<on_remove<type>>().size();
		  require(0) == state.filter<type>().size();
		  token = state.declare([&lock, &total_pack2](psl::ecs::info_t& info,
													  pack_t<policy, access, entity_t, on_remove<type>> pack1,
													  pack_t<policy, access, entity_t, filter<type>> pack2) {
			  std::lock_guard<std::mutex> guard(lock);
			  total_pack2 += pack1.size();

			  require(pack2.size()) == 0;
		  });

		  // tick #3
		  state.tick(std::chrono::duration<float>(0.1f));
		  expect(total_pack2) == e_list2.size();
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
		  auto e_list2 {state.create(static_cast<entity_t::size_type>(40))};
		  psl::array<type> values;
		  values.resize(e_list2.size());
		  std::iota(std::begin(values), std::end(values), 0);
		  ;
		  state.add_components<type>(e_list2, values);
		  auto expected = e_list2.size();

		  std::mutex lock {};

		  state.declare([&expected, &lock](psl::ecs::info_t& info, pack_t<policy, access, entity_t, type> pack) {
			  size_t removed {0};
			  psl::array<entity_t> entities;
			  for(auto [e, i] : pack) {
				  require(type(static_cast<entity_t::size_type>(e))) == i;
				  if(std::rand() % 2 == 0) {
					  entities.emplace_back(e);
					  removed++;
				  }
			  }
			  info.command_buffer.remove_components<type>(entities);

			  std::lock_guard<std::mutex> guard(lock);
			  expected -= removed;
		  });

		  while(expected > 0) state.tick(std::chrono::duration<float>(0.1f));
	  };


	  section<"continuous removal from external">() = [&]() {
		  auto e_list2 {state.create(static_cast<entity_t::size_type>(40))};
		  psl::array<type> values;
		  values.resize(e_list2.size());
		  std::iota(std::begin(values), std::end(values), 0);
		  state.add_components<type>(e_list2, values);
		  auto expected = e_list2.size();

		  size_t total = 0;
		  std::mutex lock {};

		  state.declare([&total, &lock](psl::ecs::info_t& info, pack_t<policy, access, entity_t, type> pack) {
			  for(auto [e, i] : pack) {
				  require(type(static_cast<entity_t::size_type>(e))) == i;
			  }
			  std::lock_guard<std::mutex> guard(lock);
			  total += pack.size();
		  });


		  while(expected > 0) {
			  state.tick(std::chrono::duration<float>(0.1f));
			  require(total) == expected;
			  total	   = 0;
			  auto mid = std::partition(std::begin(e_list2), std::end(e_list2), [](auto e) { return std::rand() % 2; });
			  state.remove_components<type>(
				psl::array_view<entity_t> {mid, static_cast<size_t>(std::distance(mid, std::end(e_list2)))});
			  expected -= std::distance(mid, std::end(e_list2));
			  e_list2.erase(mid, std::end(e_list2));
		  }
	  };

	  section<"continuous addition from within systems">() = [&]() {
		  auto e_list2 {state.create(static_cast<entity_t::size_type>(40))};
		  psl::array<type> values;
		  values.resize(e_list2.size());
		  std::iota(std::begin(values), std::end(values), 0);
		  state.add_components<type>(e_list2, values);
		  auto expected = e_list2.size();

		  state.declare([&expected](psl::ecs::info_t& info, pack_t<psl::ecs::full_t, access, entity_t, type> pack) {
			  require(pack.size()) == expected;

			  for(auto [e, i] : pack) {
				  require(type(static_cast<entity_t::size_type>(e))) == i;
			  }
			  auto new_count				= static_cast<entity_t::size_type>(std::rand() % 20);
			  psl::array<entity_t> entities = info.command_buffer.create(new_count);
			  psl::array<type> values;
			  values.resize(entities.size());
			  std::iota(std::begin(values), std::end(values), type(expected));
			  info.command_buffer.add_components<type>(entities, values);
			  expected += new_count;
		  });

		  while(expected <= 1'000) {
			  state.tick(std::chrono::duration<float>(0.1f));
		  }
	  };

	  section<"continuous addition from external">() = [&]() {
		  auto e_list2 {state.create(static_cast<entity_t::size_type>(40))};
		  {
			  psl::array<type> values;
			  values.resize(e_list2.size());
			  std::iota(std::begin(values), std::end(values), 0);
			  state.add_components<type>(e_list2, values);
		  }
		  auto expected = e_list2.size();

		  size_t total = 0;
		  std::mutex lock {};

		  state.declare([&total, &lock](psl::ecs::info_t& info, pack_t<policy, access, entity_t, type> pack) {
			  for(auto [e, i] : pack) {
				  require(type(static_cast<entity_t::size_type>(e))) == i;
			  }
			  std::lock_guard<std::mutex> guard(lock);
			  total += pack.size();
		  });

		  while(expected <= 1'000) {
			  state.tick(std::chrono::duration<float>(0.1f));
			  require(total) == expected;
			  total = 0;

			  auto new_count				= static_cast<entity_t::size_type>(std::rand() % 20);
			  psl::array<entity_t> entities = state.create(new_count);
			  psl::array<type> values;
			  values.resize(entities.size());
			  std::iota(std::begin(values), std::end(values), type(expected));
			  state.add_components<type>(entities, values);
			  expected += new_count;
		  }
	  };

	  section<"simple iterations">() = [&]() {
		  auto e_list1 {state.create(static_cast<entity_t::size_type>(10))};
		  auto e_list2 {state.create(static_cast<entity_t::size_type>(40))};
		  auto e_list3 {state.create(static_cast<entity_t::size_type>(50))};
		  state.add_components<float>(e_list1);
		  state.add_components<type>(e_list1);
		  auto system_id = state.declare(float_iteration_test<policy, access, float, type>);
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

	  section<"preseed_tag">() = [&]() {
		  auto e_list {state.create<type>(10)};
		  state.declare<"fullpack-from-start">(
			[](psl::ecs::info_t& info, pack_t<policy, access, entity_t, type> pack) { require(pack.size()) == 10; });
		  state.tick(std::chrono::duration<float>(0.1f));
		  auto invocation_count = 0;

		  // thanks to the preseed tag this system will always have the previous entities present in the pack
		  // "as-if" they were added in the current tick
		  state.declare<"on-add-delayed-preseed">(
			[&](psl::ecs::info_t& info, pack_t<policy, access, entity_t, type, on_add<preseed_tag, type>> pack) {
				++invocation_count;
				require(pack.size()) == (invocation_count == 1 ? 10 : 0);
			});
		  // this will not have the preseed tag, so it will only have the entities that were added in this tick (or
		  // later)
		  state.declare<"on-add-delayed">(
			[](psl::ecs::info_t& info, pack_t<policy, access, entity_t, type, on_add<type>> pack) {
				require(pack.size()) == 0;
			});

		  // other filters implicitly have the preseed tag (when it is applicable).
		  state.declare<"fullpack-delayed">(
			[](psl::ecs::info_t& info, pack_t<policy, access, entity_t, type> pack) { require(pack.size()) == 10; });
		  state.tick(std::chrono::duration<float>(0.1f));
		  state.tick(std::chrono::duration<float>(0.1f));
		  state.declare<"on-add-delayed-preseed_2">(
			[](psl::ecs::info_t& info, pack_t<policy, access, entity_t, type, on_add<preseed_tag, type>> pack) {
				require(pack.size()) == 10;
			});
		  state.tick(std::chrono::duration<float>(0.1f));
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
  suite<"filtering over multiple frames", "ecs", "psl", "regression">()
	.templates<float_tpack, policy_tpack, access_tpack>() = []<typename type, typename policy, typename access>() {
	  state_t state {1u};
	  section<"regression 1">() = [&] {
		  // issue: non-unique entry in filtering operation
		  // order of operations:
		  //  - add 1 entity -> becomes "modified entity" for filtering/ecs
		  //  - when ticking system, modify the entity again
		  //  - next tick when the results are collabed it will duplicate the entity
		  //    in the filtering for system's filtering -> issue

		  size_t count = 0;
		  std::mutex lock {};
		  state.declare([&](info_t& info, pack_t<policy, access, entity_t, type> pack) {
			  if(pack.empty())
				  return;
			  info.command_buffer.add_components<int>(pack, {0});

			  std::lock_guard guard {lock};
			  count += pack.size();
		  });

		  auto entities = state.create<type>(static_cast<entity_t::size_type>(1));
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

		  auto entities0 = state.create<type>(static_cast<entity_t::size_type>(1));
		  auto entities1 = state.create(static_cast<entity_t::size_type>(1));
		  state.remove_components<type>(entities0);
		  expect(state.filter<type>().size()) == 0;
		  expect(state.filter<on_remove<type>>().size()) == 1;
		  state.add_components<type>(entities1);
		  expect(state.filter<type>().size()) == 1;
		  expect(state.filter<on_add<type>>().size()) == 1;
		  expect(state.filter<on_remove<type>>().size()) == 1;
		  state.declare([&](info_t& info, pack_t<psl::ecs::full_t, access, entity_t, type> pack) {
			  require(pack.size()) == 1;
			  expect(static_cast<entity_t::size_type>(pack.template get<entity_t>()[0])) == 1;
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
	state_a.create(static_cast<entity_t::size_type>(200), [&counter](int& value) { value = counter++; });
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
		require(static_cast<entity_t::size_type>(entities[i])) == components_a[i];
	}

	psl::format::container container_b {};
	serializer.serialize<psl::serialization::encode_to_format>(state_b, container_b);

	require(container_b.to_string()) == container_a.to_string();
};

struct foo {
	static constexpr auto prototype() -> foo {
		return foo {10};
	}
	int value;
};

auto t10 = suite<"ecs prototype support", "ecs", "psl">() = []() {
	psl::ecs::state_t state {};
	auto entity = state.create<foo>(1);
	require(state.get<foo>(entity[0]).value) == 10;
};

auto t11 = suite<"ecs versioning", "ecs", "psl">() = []() {
	// this test will load an outdated version of the `updated_component` (see `updated_component_v0`)
	// and we'll verify if the data migration went correctly. If all went fine the value in the component
	// should be equal to the entity id associated with the component.
	psl::ecs::state_t state {};
	psl::serialization::serializer s {};
	s.deserialize<psl::serialization::decode_from_format>(state, "tdata/outdated.txt");

	auto entities	= state.all_entities();
	auto components = state.get_component<updated_component>(entities);

	for(auto e : entities) {
		auto value = components[static_cast<psl::ecs::entity_t::size_type>(e)].value;
		require(value == (int)static_cast<psl::ecs::entity_t::size_type>(e));
	}
};

auto t12 = suite<"ecs restricted mutability", "ecs", "psl">() = []() {
	psl::ecs::state_t state {};

	static_assert(psl::ecs::IsRestrictedMutable<foo_restricted>);
	auto entities		   = state.create<foo_restricted>(static_cast<entity_t::size_type>(5), {0, 0, false});
	auto modified_entities = psl::array_view<entity_t> {std::begin(entities), 2};	 // only modify 2 of the 5 entities
	foo_restricted mutated_values {5, 3, true};
	state.mutate_components<foo_restricted>(modified_entities, mutated_values);
	bool has_mutated {true};

	state.declare([&has_mutated, &mutated_values](
					psl::ecs::info_t& info,
					psl::ecs::pack_indirect_full_t<const foo_restricted, psl::ecs::on_mutate<foo_restricted>> pack) {
		require(pack.size()) == ((has_mutated) ? 2 : 0);

		for(auto [value, mutator] : pack) {
			require(value.value1) == mutated_values.value1;
			require(value.value2) == mutated_values.value2;
			require(value.value3) == mutated_values.value3;

			require(mutator.has_mutated<&foo_restricted::value1>()) == has_mutated;
			require(mutator.has_mutated<&foo_restricted::value2>()) == has_mutated;
			require(mutator.has_mutated<&foo_restricted::value3>()) == has_mutated;
		}
	});

	state.declare([](psl::ecs::info_t& info, psl::ecs::pack_indirect_full_t<const foo_restricted> pack) {
		require(pack.size()) == 5;
		auto entries = pack.get<const foo_restricted>();
		require(entries[2].value1) == 0;
	});

	state.tick(std::chrono::duration<float>(1.0f));

	has_mutated = false;
	state.tick(std::chrono::duration<float>(1.0f));

	mutated_values = {99, 2, false};
	state.mutate_components<foo_restricted>(modified_entities, mutated_values);
	has_mutated = true;
	state.tick(std::chrono::duration<float>(1.0f));
};

auto t13 = suite<"ecs restricted mutability - systems", "ecs", "psl">() = []() {
	psl::ecs::state_t state {};
	auto entities = state.create<foo_restricted>(static_cast<entity_t::size_type>(5), {0, 0, false});

	state.declare([](psl::ecs::info_t& info,
					 psl::ecs::pack_indirect_full_t<const foo_restricted, psl::ecs::on_mutate<foo_restricted>> pack) {
		require(pack.size()) == (info.tick == 0 ? 0 : 5);

		for(auto [value, mutator] : pack) {
			require(value.value1) == (int)info.tick - 1;
			require(value.value2) == 3.0f * (info.tick - 1);
			require(value.value3) == true;

			require(mutator.has_mutated<&foo_restricted::value1>()) == (info.tick == 1 ? false : true);
			require(mutator.has_mutated<&foo_restricted::value2>()) == (info.tick == 1 ? false : true);
			require(mutator.has_mutated<&foo_restricted::value3>()) == (info.tick == 1 ? true : false);
		}
	});

	state.declare([](psl::ecs::info_t& info, psl::ecs::pack_indirect_full_t<entity_t, const foo_restricted> pack) {
		require(pack.size()) == 5;
		info.command_buffer.mutate_components<foo_restricted>(pack,
															  foo_restricted {(int)info.tick, 3.0f * info.tick, true});
	});
	state.tick(std::chrono::duration<float>(1.0f));
	state.tick(std::chrono::duration<float>(1.0f));
	state.tick(std::chrono::duration<float>(1.0f));
	state.tick(std::chrono::duration<float>(1.0f));
};

auto t14 = suite<"entity_relations", "ecs", "psl">() = []() {
	psl::ecs::state_t state {};

	section<"state_t">() = [&]() {
		auto entities = state.create(static_cast<entity_t::size_type>(10));
		psl::array<entity_t> children {std::next(entities.begin()), entities.end()};
		state.set_parent(entities[0], children);
		require(state.has_children(entities[0]));
		require(std::all_of(std::begin(children), std::end(children), [&](auto e) {
			return (state.has_parent(e)) && state.get_parent(e) == entities[0];
		}));

		auto parents_children = state.get_children(entities[0]);
		require(parents_children.size()) == children.size();
		require(std::equal(std::begin(parents_children), std::end(parents_children), std::begin(children)));

		state.unparent(children[0]);
		require(!state.has_parent(children[0]));

		children	  = state.get_children(entities[0]);
		auto siblings = state.get_siblings(children[0]);
		require(children.size()) == 8;
		require(siblings.size()) == 7;
		// note we go to the next element because siblings does not include the element itself
		require(std::equal(std::begin(siblings), std::end(siblings), std::next(std::begin(children))));

		require(!state.is_parent_of(entities[0], entities[1]));
		require(!state.is_sibling(entities[1], children[0]));
		require(state.is_sibling(children[1], children[0]));

		state.set_parent(entities[1], entities[0]);

		require(state.get_root(children[0])) == entities[1];
		require(state.get_parent(children[0])) == entities[0];
		require(state.get_parent(entities[0])) == entities[1];

		auto root_children = state.get_children(entities[1], true);
		require(root_children.size()) == 1;

		auto root_all_children = state.get_all_children(entities[1]);
		require(root_all_children.size()) == 9;

		require(state.is_indirect_parent_of(entities[1], children[0]));
		require(state.is_indirect_parent_of(entities[0], children[0]));
		require(state.is_parent_of(entities[0], children[0]));
		require(!state.is_parent_of(entities[1], children[0]));
		require(state.is_root(entities[1]));
		require(!state.is_root(entities[0]));

		auto all_parents = state.get_all_parents(children[0]);
		require(all_parents.size()) == 2;
		std::sort(std::begin(all_parents), std::end(all_parents));
		require(all_parents[0]) == entities[0];
		require(all_parents[1]) == entities[1];

		all_parents = state.get_all_parents(entities[0]);
		require(all_parents.size()) == 1;
		require(all_parents[0]) == entities[1];

		all_parents = state.get_all_parents(entities[1]);
		require(all_parents.size()) == 0;


		require(state.get_siblings(entities[0]).size()) == 0;
		require(state.get_siblings(entities[1]).size()) == 0;
	};

	section<"systems">() = [&]() {
		auto entities = state.create<position>(20);
		state.set_parent(entities[0], entities[1]);
		state.set_parent(entities[1],
						 psl::array_view<entity_t> {std::next(entities.begin(), 2), std::next(entities.begin(), 10)});

		// Layer 1 and 2 satisy this query (they are the ones that got reparented), but as we additionally filter for
		// self we get layer 2 as well in addition to layer 0 and 1 (direct_parent of layer 2 is layer 1, and layer 1
		// is / layer 0)
		state.declare(
		  transient_system_tag,
		  [&entities](
			psl::ecs::info_t& info,
			psl::ecs::pack_indirect_full_t<
			  entity_t,
			  const position,
			  psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::reparented>,
			  psl::ecs::get_relationship<entity_relationship::direct_parent | entity_relationship::self>> pack) {
			  require(pack.size()) == 10;

			  require(std::equal(std::begin(pack.template get<entity_t>()),
								 std::end(pack.template get<entity_t>()),
								 std::begin(entities)));
		  });

		// Layer 1 and 2 satisfies this query, and their direct parents would be layer 0 and 1 respectively
		state.declare(
		  transient_system_tag,
		  [&entities](
			psl::ecs::info_t& info,
			psl::ecs::pack_indirect_full_t<entity_t,
										   const position,
										   psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::reparented>,
										   psl::ecs::get_relationship<entity_relationship::direct_parent>> pack) {
			  require(pack.size()) == 2;
			  require(pack.template get<entity_t>()[0]) == entities[0];
			  require(pack.template get<entity_t>()[1]) == entities[1];
		  });

		// The only ones satisfying this query are layer 0 & 1, and their children would be layer 1 & 2
		state.declare(
		  transient_system_tag,
		  [&entities](
			psl::ecs::info_t& info,
			psl::ecs::pack_indirect_full_t<entity_t,
										   const position,
										   psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::child_added>,
										   psl::ecs::get_relationship<entity_relationship::direct_children>> pack) {
			  require(pack.size()) == 9;
			  require(std::equal(std::begin(pack.template get<entity_t>()),
								 std::end(pack.template get<entity_t>()),
								 std::next(std::begin(entities), 1)));
		  });

		// the last layer is the only ones who have siblings, but the parents are the ones that will have a
		// child_changed event
		state.declare(
		  transient_system_tag,
		  [](psl::ecs::info_t& info,
			 psl::ecs::pack_indirect_full_t<
			   entity_t,
			   const position,
			   psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::child_changed>,
			   psl::ecs::get_relationship<entity_relationship::siblings>> pack) { require(pack.size()) == 0; });

		// as only the last layer that got reparented has siblings, we expect only those 8 entities to be present
		state.declare(
		  transient_system_tag,
		  [&entities](
			psl::ecs::info_t& info,
			psl::ecs::pack_indirect_full_t<entity_t,
										   const position,
										   psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::reparented>,
										   psl::ecs::get_relationship<entity_relationship::siblings>> pack) {
			  require(pack.size()) == 8;
			  require(std::equal(std::begin(pack.template get<entity_t>()),
								 std::end(pack.template get<entity_t>()),
								 std::next(std::begin(entities), 2)));
		  });

		state.tick(std::chrono::duration<float>(1.0f));

		// with preseed_tag we will now filter for all those _with_ children and return ourselves.
		// in our current case that is layer 0 & 1
		state.declare(
		  transient_system_tag,
		  [&entities](psl::ecs::info_t& info,
					  psl::ecs::pack_indirect_full_t<
						entity_t,
						const position,
						psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::child_added, preseed_tag>,
						psl::ecs::get_relationship<entity_relationship::self>> pack) {
			  require(pack.size()) == 2;
			  require(pack.template get<entity_t>()[0]) == entities[0];
			  require(pack.template get<entity_t>()[1]) == entities[1];
		  });

		// same as preceeding, but instead we get the direct parents. this results in only layer 0 getting returned
		state.declare(
		  transient_system_tag,
		  [&entities](psl::ecs::info_t& info,
					  psl::ecs::pack_indirect_full_t<
						entity_t,
						const position,
						psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::child_added, preseed_tag>,
						psl::ecs::get_relationship<entity_relationship::direct_parent>> pack) {
			  require(pack.size()) == 1;
			  require(pack.template get<entity_t>()[0]) == entities[0];
		  });

		// like the above test, but with additionally self filtering, so we get layer 0 and layer 1
		state.declare(
		  transient_system_tag,
		  [&entities](
			psl::ecs::info_t& info,
			psl::ecs::pack_indirect_full_t<
			  entity_t,
			  const position,
			  psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::child_added, preseed_tag>,
			  psl::ecs::get_relationship<entity_relationship::direct_parent | entity_relationship::self>> pack) {
			  require(pack.size()) == 2;
			  require(pack.template get<entity_t>()[0]) == entities[0];
			  require(pack.template get<entity_t>()[1]) == entities[1];
		  });

		// catches all (except floating root entities) in the current setup
		state.declare(
		  transient_system_tag,
		  [&entities](
			psl::ecs::info_t& info,
			psl::ecs::pack_indirect_full_t<
			  entity_t,
			  const position,
			  psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::child_added, preseed_tag>,
			  psl::ecs::get_relationship<entity_relationship::all_children | entity_relationship::self>> pack) {
			  require(pack.size()) == 10;
			  require(std::equal(std::begin(pack.template get<entity_t>()),
								 std::end(pack.template get<entity_t>()),
								 std::begin(entities)));
		  });

		// similar to the child_added test earlier, but from the perspective of reparenting
		state.declare(
		  transient_system_tag,
		  [&entities](psl::ecs::info_t& info,
					  psl::ecs::pack_indirect_full_t<
						entity_t,
						const position,
						psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::reparented, preseed_tag>,
						psl::ecs::get_relationship<entity_relationship::direct_parent>> pack) {
			  require(pack.size()) == 2;
			  require(pack.template get<entity_t>()[0]) == entities[0];
			  require(pack.template get<entity_t>()[1]) == entities[1];
		  });

		// no-preseed_tag version of the preceeding test as a sanity check
		state.declare(
		  transient_system_tag,
		  [](psl::ecs::info_t& info,
			 psl::ecs::pack_indirect_full_t<entity_t,
											const position,
											psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::reparented>,
											psl::ecs::get_relationship<entity_relationship::direct_parent>> pack) {
			  require(pack.size()) == 0;
		  });
		state.tick(std::chrono::duration<float>(1.0f));

		// from here on out we have 2 trees;
		// 0 -> 1 -> [3..9] and 10 -> 2
		// that means 11 entities have relations, and 9 are orphans
		state.set_parent(entities[10], entities[2]);

		// get all siblings, this results in [3..9] being returned due to the preseed_tag
		state.declare(
		  transient_system_tag,
		  [&entities](psl::ecs::info_t& info,
					  psl::ecs::pack_indirect_full_t<
						entity_t,
						const position,
						psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::reparented, preseed_tag>,
						psl::ecs::get_relationship<entity_relationship::siblings>> pack) {
			  require(pack.size()) == 7;
			  require(std::equal(std::begin(pack.template get<entity_t>()),
								 std::end(pack.template get<entity_t>()),
								 std::next(std::begin(entities), 3)));
		  });

		// same as previous but without preseed_tag results in 0 siblings
		state.declare(
		  transient_system_tag,
		  [&entities](
			psl::ecs::info_t& info,
			psl::ecs::pack_indirect_full_t<entity_t,
										   const position,
										   psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::reparented>,
										   psl::ecs::get_relationship<entity_relationship::siblings>> pack) {
			  require(pack.size()) == 0;
		  });

		// without the preseed_tag the only one who had a reparenting event was entity 2, and as we get its parent
		// we get entity 10 back
		state.declare(
		  transient_system_tag,
		  [&entities](
			psl::ecs::info_t& info,
			psl::ecs::pack_indirect_full_t<entity_t,
										   const position,
										   psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::reparented>,
										   psl::ecs::get_relationship<entity_relationship::direct_parent>> pack) {
			  require(pack.size()) == 1;
			  require(pack.template get<entity_t>()[0]) == entities[10];
		  });

		// all entities w/ preseed_tag who have a child, results in entities 0, 1, and 10
		state.declare(
		  transient_system_tag,
		  [&entities](psl::ecs::info_t& info,
					  psl::ecs::pack_indirect_full_t<
						entity_t,
						const position,
						psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::child_added, preseed_tag>,
						psl::ecs::get_relationship<entity_relationship::self>> pack) {
			  require(pack.size()) == 3;
			  require(pack.template get<entity_t>()[0]) == entities[0];
			  require(pack.template get<entity_t>()[1]) == entities[1];
			  require(pack.template get<entity_t>()[2]) == entities[10];
		  });

		// all direct parents w/ preseed_tag results in the same result as the previous filter test
		state.declare(
		  transient_system_tag,
		  [&entities](psl::ecs::info_t& info,
					  psl::ecs::pack_indirect_full_t<
						entity_t,
						const position,
						psl::ecs::on_hierarchy_change<psl::ecs::hierarchy_change_event::reparented, preseed_tag>,
						psl::ecs::get_relationship<entity_relationship::direct_parent>> pack) {
			  require(pack.size()) == 3;
			  require(pack.template get<entity_t>()[0]) == entities[0];
			  require(pack.template get<entity_t>()[1]) == entities[1];
			  require(pack.template get<entity_t>()[2]) == entities[10];
		  });
		state.tick(std::chrono::duration<float>(1.0f));
	};

	return;
	section<"entity_relationship_data_t">() = [&]() {
		auto entities = state.create<position>(20);
		state.set_parent(entities[0], entities[1]);
		state.set_parent(entities[1],
						 psl::array_view<entity_t> {std::next(entities.begin(), 2), std::next(entities.begin(), 10)});

		state.declare(
		  transient_system_tag,
		  [&entities](psl::ecs::info_t& info,
					  psl::ecs::pack_indirect_full_t<entity_t, const position, const entity_relationship_data_t> pack) {
			  require(pack.size()) == 20;
			  {
				  // first entity has no parent, and one child
				  auto& data = pack.template get<entity_relationship_data_t const>()[0];
				  require(data.is_root()) == true;
				  require(data.has_parent()) == false;
				  require(data.has_children()) == true;
				  require(data.has_siblings()) == false;
				  require(data.children_count()) == 1;
				  require(data.children()[0]) == entities[1];
			  }
			  {
				  // second entity has a parent, and 8 children
				  auto& data = pack.template get<entity_relationship_data_t const>()[1];
				  require(data.is_root()) == false;
				  require(data.has_parent()) == true;
				  require(data.has_children()) == true;
				  require(data.has_siblings()) == false;
				  require(data.parent()) == entities[0];
				  require(data.children_count()) == 8;
				  require(std::equal(
					std::begin(data.children()), std::end(data.children()), std::next(std::begin(entities), 2)));
			  }
			  {
				  for(auto i = 2; i < 10; ++i) {
					  // all entities in the range [2..9] have a parent, no children, and 7 siblings (8 with themselves
					  // included)
					  auto& data = pack.template get<entity_relationship_data_t const>()[i];
					  require(data.is_root()) == false;
					  require(data.has_parent()) == true;
					  require(data.has_children()) == false;
					  require(data.has_siblings()) == true;
					  require(data.parent()) == entities[1];
					  require(data.children_count()) == 0;
					  require(data.siblings_count()) == 7;
					  require(std::equal(
						std::begin(data.siblings()), std::end(data.siblings()), std::next(std::begin(entities), 2)));
					  auto siblings_excluding_self = data.siblings_excluding_self();
					  require(std::find(std::begin(siblings_excluding_self),
										std::end(siblings_excluding_self),
										entities[i]) == std::end(siblings_excluding_self));
				  }
			  }
			  {
				  // the remaining entities [10..19] have no parent, no children, and no siblings
				  for(auto i = 10; i < 20; ++i) {
					  auto& data = pack.template get<entity_relationship_data_t const>()[i];
					  require(data.is_root()) == true;
					  require(data.has_parent()) == false;
					  require(data.has_children()) == false;
					  require(data.has_siblings()) == false;
					  require(data.children_count()) == 0;
					  require(data.siblings_count()) == 0;
				  }
			  }
		  });
		state.tick(std::chrono::duration<float>(1.0f));
	};
};

}	 // namespace
