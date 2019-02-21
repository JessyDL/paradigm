#pragma once
#include "stdafx_benchmarks.h"
#include "runner.h"

#include "../../psl/inc/ecs/state.h"

struct position
{
	std::uint64_t x;
	std::uint64_t y;
};

struct rotation
{
	std::uint64_t x;
	std::uint64_t y;
};

template <size_t C = 0, typename... Ts>
struct mock_data
{
	mock_data() { entities = state.create<Ts...>(C); }
	psl::array<psl::ecs::entity> entities;
	psl::ecs::state state;
};
template <size_t C>
using mock_data_position = mock_data<C, position>;

template <size_t C>
using mock_data_position_rotation = mock_data<C, position, rotation>;


//BENCHMARK("Looping 1000 times creating 1000 entities and deleting a random number of entities", mock_data<>)
//{
//	psl::ecs::state state{};
//	for(int i = 0; i < 10000; i++)
//	{
//		state.create<position>(10000);
//		psl::array<psl::ecs::entity> range{state.entities<position>()};
//		std::for_each(std::begin(range), std::end(range), [&state](auto e) {
//			if(e % 2 == 0) state.destroy(e);
//		});
//	}
//}

BENCHMARK("Constructing 1000000 entities", mock_data<>)
{
	for(std::uint64_t i = 0; i < 1000000L; i++)
	{
		data.state.create();
	}
}

BENCHMARK("Constructing 1000000 entities at once", mock_data<>) { data.state.create(1000000L); }


BENCHMARK("Constructing 1000000 entities /w one component at once", mock_data<>)
{
	data.state.create<position>(1000000L);
}

BENCHMARK("Destroying 1000000 entities", mock_data<1000000L>)
{
	for(std::uint32_t i = 0; i < 1000000L; i++)
	{
		data.state.destroy(i);
	}
}

BENCHMARK("Destroying 1000000 entities at once", mock_data<1000000L>)
{
	data.state.destroy(psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>>{{0u, 1000000u}});
}

BENCHMARK("Destroying 999999 entities out of 1000000 at once", mock_data<1000000L>)
{
	data.state.destroy(psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>>{{0u, 999999u}});
}

BENCHMARK("Removing 1000000 components from 1000000 entities", mock_data_position<1000000L>)
{
	data.state.remove_components(data.entities);
}

BENCHMARK("Removing 1000000 components from 1000001 entities", mock_data_position_rotation<1000001L>)
{
	data.entities.resize(data.entities.size() - 1);
	data.state.remove_components(data.entities);
}

BENCHMARK("Iterating over 1000000 entities that have one component", mock_data_position_rotation<1000000L>, true)
{
	auto view = data.state.view<position>();
	std::for_each(std::begin(view), std::end(view), [](position& e) { e.x = 5; });
}

BENCHMARK("Iterating over 1000000 entities that have one const component", mock_data_position_rotation<1000000L>, true)
{
	auto view	= data.state.view<position>();
	size_t count = 0;
	std::for_each(std::begin(view), std::end(view), [&count](const position& e) { count++; });
}
