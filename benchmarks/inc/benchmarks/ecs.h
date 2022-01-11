#pragma once
#include "stdafx_benchmarks.h"
#include "runner.h"

#include "../../psl/inc/ecs/state.h"

using namespace psl::ecs;

struct position
{
	std::uint64_t x;
	std::uint64_t y;
};

struct velocity
{
	std::uint64_t x;
	std::uint64_t y;
};


struct rotation
{
	std::uint64_t x;
	std::uint64_t y;
};

struct health
{
	float value;
};

struct lifetime
{
	int value;
};


template <size_t C = 0, typename... Ts>
struct mock_data
{
	mock_data() { entities = state.create<Ts...>(C); }
	psl::array<psl::ecs::entity> entities;
	psl::ecs::state_t state;
};
template <size_t C>
using mock_data_position = mock_data<C, position>;

template <size_t C>
using mock_data_position_rotation = mock_data<C, position, rotation>;

template <size_t C>
struct mock_data_pos_rot
{
	mock_data_pos_rot()
	{
		entities = state.create<rotation>(C);
		psl::array_view<psl::ecs::entity> other(std::begin(entities), C / 2);
		state.add_components<position>(other);
	}
	psl::array<psl::ecs::entity> entities;
	psl::ecs::state_t state;
};

template <size_t C>
struct mock_data_trivial_system
{
	mock_data_trivial_system() { entities = state.create<rotation, position, velocity>(C); }
	psl::array<psl::ecs::entity> entities;
	psl::ecs::state_t state;
};

template<size_t C>
using full_mock_data = mock_data<C, position, rotation, velocity, health, lifetime>;




BENCHMARK("Constructing 1.000.000 entities", mock_data<>)
{
	for(std::uint64_t i = 0; i < 1000000L; i++)
	{
		data.state.create();
	}
}

BENCHMARK("Constructing 1.000.000 entities at once", mock_data<>) { data.state.create(1000000L); }


BENCHMARK("Constructing 1.000.000 entities /w one component at once", mock_data<>)
{
	data.state.create<position>(1000000L);
}

BENCHMARK("Destroying 1.000.000 entities", mock_data<1000000L>)
{
	for(std::uint32_t i = 0; i < 1000000L; i++)
	{
		data.state.destroy(i);
	}
}

BENCHMARK("Destroying 1.000.000 entities at once", mock_data<1000000L>)
{
	data.state.destroy(psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>>{{0u, 1000000u}});
}

BENCHMARK("Destroying 999.999 entities out of 1000000 at once", mock_data<1000000L>)
{
	data.state.destroy(psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>>{{0u, 999999u}});
}

BENCHMARK("Removing 1.000.000 components from 1.000.000 entities", mock_data_position<1000000L>)
{
	data.state.remove_components(data.entities);
}

BENCHMARK("Removing 1.000.000 components from 1.000.001 entities", mock_data_position_rotation<1000001L>)
{
	data.entities.resize(data.entities.size() - 1);
	data.state.remove_components(data.entities);
}

BENCHMARK("Iterating over 1.000.000 entities that have one component", mock_data_position_rotation<1000000L>, true)
{
	auto view = data.state.view<position>();
	std::for_each(std::begin(view), std::end(view), [](position& e) { e.x = 5; });
}

BENCHMARK("Iterating over 1.000.000 entities that have one const component", mock_data_position_rotation<1000000L>, true)
{
	auto view	= data.state.view<position>();
	size_t count = 0;
	std::for_each(std::begin(view), std::end(view), [&count](const position& e) { count++; });
}

BENCHMARK(
	"Loop over 500.000 position components, out of 1.000.000 entities that all have a rotation component, and half "
	"have position component",
	mock_data_pos_rot<1000000L>, true)
{
	auto view = data.state.view<position>();
	std::for_each(std::begin(view), std::end(view), [](position& e) {});
}

BENCHMARK(
	"Loop over 500.000 const position components, out of 1.000.000 entities that all have a rotation component, and "
	"half "
	"have position component",
	mock_data_pos_rot<1000000L>, true)
{
	auto view	= data.state.view<position>();
	size_t count = 0;
	std::for_each(std::begin(view), std::end(view), [&count](const position& e) { count++; });
}

BENCHMARK("Running a trivial system that moves components around", mock_data_trivial_system<100000>)
{
	auto system = [](psl::ecs::info& info, psl::ecs::pack<position, const velocity> data) {
		for(auto [pos, vel] : data)
		{
			pos.x += (uint64_t)((float)vel.x * info.dTime.count());
			pos.y += (uint64_t)((float)vel.y * info.dTime.count());
		}
	};

	data.state.declare(system);
	data.state.tick(std::chrono::duration<float>(0.1f));
}

BENCHMARK("filter 1.000.000 filter<T>", mock_data_trivial_system<1000000>)
{
	data.state.filter<position>();
}

BENCHMARK("filter 1.000.000 on_add<T>", mock_data_trivial_system<1000000>)
{
	data.state.filter<on_add<position>>();
}

BENCHMARK("filter  1.000.000 on_combine<T, U>", mock_data_trivial_system<1000000>)
{
	data.state.filter<on_combine<velocity, position>>();
}

BENCHMARK("filter 1.000.000 filter<T, U>", mock_data_trivial_system<1000000>)
{
	data.state.filter<position, velocity>();
}

BENCHMARK("filter 1.000.000 on_add<T, U>", mock_data_trivial_system<1000000>)
{
	data.state.filter<on_add<position>, on_add<velocity>>();
}

BENCHMARK("filter  1.000.000 on_combine<T, U, V>", mock_data_trivial_system<1000000>)
{
	data.state.filter<on_combine<velocity, position, rotation>>();
}

BENCHMARK("filter 1.000.000 filter<T, U, V>", mock_data_trivial_system<1000000>)
{
	data.state.filter<position, velocity, rotation>();
}

BENCHMARK("filter 1.000.000 filter<T, U, V, X, Y>", full_mock_data<1000000>)
{
	data.state.filter<position, velocity, rotation, lifetime, health>();
}


BENCHMARK("Looping 1.000 times creating 1.000 entities and deleting a random number of entities", mock_data<>)
{
	for(int i = 0; i < 1000; i++)
	{
		data.state.create<position>(1000);
		psl::array<psl::ecs::entity> range{data.state.entities<position>()};
		std::for_each(std::begin(range), std::end(range), [&data, i](auto e)
					  {
						  if((e + i) % 2 == 0) data.state.destroy(e);
					  });
		data.state.tick(std::chrono::duration<float>(0.1f));
	}
}