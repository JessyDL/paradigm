#include "stdafx_tests.h"
#include "ecs.h"

using namespace core::ecs;
using namespace tests::ecs;


namespace tests::ecs
{
struct float_system
{
	core::ecs::vector<const float> m_Floats;
	core::ecs::vector<int> m_Ints;
	core::ecs::vector<core::ecs::entity> m_Entities;

	void announce(core::ecs::state& state)
	{
		state.register_dependency(*this, {m_Entities, m_Floats, m_Ints});
	}

	void tick(core::ecs::state& state, std::chrono::duration<float> dTime)
	{
		for(size_t i = 0; i < m_Entities.size(); ++i)
		{
			m_Ints[i] += 5;
		}
	}
};
}

TEST_CASE("component_key must be unique", "[ECS]")
{
	using namespace core::ecs::details;
	auto fl_id = component_key<float>;
	auto int_id = component_key<int>;
	REQUIRE(fl_id != int_id);


	constexpr auto fl_cid = component_key<float>;
	constexpr auto int_cid = component_key<int>;
	REQUIRE(fl_cid != int_cid);


	REQUIRE(fl_cid == fl_id);
	REQUIRE(int_cid == int_id);
}

TEST_CASE("filtering", "[ECS]")
{
	state state;
	auto e_list1{ state.create(100) };
	auto e_list2{ state.create(400) };
	auto e_list3{ state.create(500) };

	SECTION("only the first 100 are given all components")
	{
		state.add_component<float>(e_list1);
		state.add_component<bool>(e_list1);
		state.add_component<int>(e_list1);
		state.add_component<char>(e_list1);
		state.add_component<std::byte>(e_list1);

		auto f = state.filter<float, bool, int, char, std::byte>();
		REQUIRE(f == e_list1);
	}

	SECTION("500 are given two component types and 100 are given 3 component types where 2 overlap")
	{
		state.add_component<int>(e_list1);
		state.add_component<char>(e_list1);
		state.add_component<bool>(e_list1);

		state.add_component<char>(e_list3);
		state.add_component<bool>(e_list3);


		auto f = state.filter<float, bool, int, char, std::byte>();
		REQUIRE(f.size() == 0);

		f = state.filter<bool, int, char>();
		REQUIRE(f == e_list1);

		f = state.filter<bool, char>();
		std::vector<entity> combination = e_list1;
		combination.reserve(e_list1.size() + e_list3.size());
		combination.insert(std::end(combination), std::begin(e_list3), std::end(e_list3));
		REQUIRE(f == combination);
		f = state.filter<char>();
		REQUIRE(f == combination);
	}
}


TEST_CASE("systems", "[ECS]")
{
	state state;
	auto e_list1{ state.create(10) };
	auto e_list2{ state.create(40) };
	auto e_list3{ state.create(50) };

	state.add_component<float>(e_list1);
	state.add_component<int>(e_list1);


	float_system fl_system{};
	state.register_system(fl_system);
	for(int i = 0; i < 10; ++i)
		state.tick();

	std::is_empty<int>::value;
	sizeof(int);
	std::vector<int> results;
	auto f = state.filter<int>(results);
	REQUIRE(results.size() == f.size());
	REQUIRE(results.size() == 10);
	for (const auto& res : results)
	{
		REQUIRE(res == 10 * 5);
	}
}