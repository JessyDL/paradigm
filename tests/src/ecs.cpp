#include "stdafx_tests.h"
#include "ecs.h"
#include "../../psl/inc/ecs/state.h"

using namespace psl::ecs;
using namespace tests::ecs;


void registration_test(info& info) {}


namespace tests::ecs
{
	struct float_system
	{
		float_system(psl::ecs::state& state) { state.declare(&float_system::tick, this); }

		void tick(info& info, psl::ecs::pack<const float, int> pack)
		{
			for(auto [fl, i] : pack)
			{
				i += 5;
			}
		}
	};
} // namespace tests::ecs

TEST_CASE("component_key must be unique", "[ECS]")
{
	using namespace psl::ecs::details;
	auto fl_id  = component_key<float>;
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
		state.create(3, float{}, size_t{});

		REQUIRE(state.filter<float>().size() + 1 == state.filter<on_add<float>>().size());
		REQUIRE(state.filter<float>().size() == state.filter<on_combine<float, size_t>>().size());
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

	size_t count {0};
	state.create(50, [&count](position& i) { i = {++count, 0}; }, float{5.0f}, psl::ecs::empty<size_t>());

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
	auto e_list1{state.create(10)};
	auto e_list2{state.create(40)};
	auto e_list3{state.create(50)};


	state.add_components<float>(e_list1);
	state.add_components<int>(e_list1);


	float_system fl_system{state};
	for(int i = 0; i < 10; ++i) state.tick(std::chrono::duration<float>(0.1f));

	std::is_empty<int>::value;
	sizeof(int);
	auto entities = state.filter<int>();
	auto results  = state.view<int>();
	REQUIRE(results.size() == entities.size());
	REQUIRE(results.size() == 10);
	for(const auto& res : results)
	{
		// REQUIRE(res == 10 * 5);
	}
}


TEST_CASE("declaring system signatures", "[ECS]")
{
	state state;
	state.declare(registration_test);
	float_system fl_system{state};
}