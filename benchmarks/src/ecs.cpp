#include "psl/ecs/state.hpp"
#include <benchmark/benchmark.h>

using namespace psl;
using namespace psl::ecs;

#define BENCHMARK_ENTITY_CREATION
#define BENCHMARK_COMPONENT_CREATION
#define BENCHMARK_FILTERING
#define BENCHMARK_SYSTEMS

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

#ifdef BENCHMARK_ENTITY_CREATION
void entity_creation(benchmark::State& gState)
{
	auto eCount = gState.range(0);

	ecs::state_t state;
	for(auto _ : gState)
	{
		gState.PauseTiming();
		state.clear();
		gState.ResumeTiming();
		state.create(eCount);
	}
}

void entity_creation_with_destruction(benchmark::State& gState)
{
	auto eCount		= gState.range(0);
	auto eHalfCount = static_cast<ecs::entity>(eCount / 2);

	ecs::state_t state;
	for(auto _ : gState)
	{
		gState.PauseTiming();
		state.clear();
		gState.ResumeTiming();
		auto ents = state.create(eCount);
		ents.erase(std::next(std::begin(ents), eHalfCount), std::end(ents));
		state.destroy(ents);
		state.create(eHalfCount);
	}
}

BENCHMARK(entity_creation)->RangeMultiplier(10)->Range(1, 1'000'000)->Unit(benchmark::kMicrosecond);
BENCHMARK(entity_creation_with_destruction)->RangeMultiplier(10)->Range(1, 1'000'000)->Unit(benchmark::kMicrosecond);
#endif

#ifdef BENCHMARK_COMPONENT_CREATION
void component_creation(benchmark::State& gState)
{
	auto eCount = gState.range(0);
	auto cCount = gState.range(1);
	ecs::state_t state;
	auto entities = state.create(eCount);

	for(auto _ : gState)
	{
		gState.PauseTiming();
		state.clear();
		gState.ResumeTiming();
		if(cCount >= 5) state.add_components<uint64_t>(entities);
		if(cCount >= 4) state.add_components<bool>(entities);
		if(cCount >= 3) state.add_components<char>(entities);
		if(cCount >= 2) state.add_components<float>(entities);

		state.add_components<int>(entities);
	}
}

void component_creation_args(benchmark::internal::Benchmark* b)
{
	for(int j = 1; j <= 5; ++j)
		for(int i = 0; i <= 6; ++i) b->ArgPair(pow(10, i), j);
}
BENCHMARK(component_creation)->Apply(component_creation_args)->Unit(benchmark::kMicrosecond);
#endif

#ifdef BENCHMARK_FILTERING
template <typename... Ts>
class filtering_fixture : public ::benchmark::Fixture
{
	const std::vector<std::vector<int>> data_constraint {{10'000, 300, 2'700, 1'200, 6'700},
														 {100'000, 3'000, 21'700, 10'200, 68'700},
														 {10'000, 300, 2'700, 3'200, 6'700},
														 {100'000, 3'000, 21'700, 30'200, 68'700}};

  public:
	void SetUp(const ::benchmark::State& gState) override
	{
		auto index		 = gState.range();
		const auto& data = data_constraint[index];
		auto eCount		 = data[0];
		auto char_beg	 = data[1];
		auto char_end	 = data[2];
		auto int_beg	 = data[3];
		auto int_end	 = data[4];

		auto entities = state.create(eCount);

		state.add_components<float>(psl::array<entity> {std::begin(entities), std::next(std::begin(entities), eCount)});
		state.add_components<char>(
		  psl::array<entity> {std::next(std::begin(entities), char_beg), std::next(std::begin(entities), char_end)});

		state.add_components(
		  psl::array<entity> {std::next(std::begin(entities), int_beg), std::next(std::begin(entities), int_end)},
		  [](int& i) { i = std::rand() % 1000; });
	}

	void filter() { state.filter<Ts...>(); }

	void order_by(benchmark::State& gState)
	{
		gState.PauseTiming();
		auto entities = state.filter<int>();
		gState.ResumeTiming();

		state.order_by<std::less<int>, int>(std::begin(entities), std::end(entities));
	}

	void on_condition(benchmark::State& gState)
	{
		gState.PauseTiming();
		auto entities = state.filter<int>();
		gState.ResumeTiming();

		state.on_condition<int>(std::begin(entities), std::end(entities), [](const int& i) { return i < 500; });
	}
	ecs::state_t state;
};

BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_int, int)(benchmark::State& gState)
{
	for(auto _ : gState)
	{
		filter();
	}
}

BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_char_float, char, float)(benchmark::State& gState)
{
	for(auto _ : gState)
	{
		filter();
	}
}
BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_char_int, char, int)(benchmark::State& gState)
{
	for(auto _ : gState)
	{
		filter();
	}
}
BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_char_int_float, char, int, float)
(benchmark::State& gState)
{
	for(auto _ : gState)
	{
		filter();
	}
}
BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_float_int_char, float, int, char)
(benchmark::State& gState)
{
	for(auto _ : gState)
	{
		filter();
	}
}

BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_order_by)
(benchmark::State& gState)
{
	for(auto _ : gState)
	{
		order_by(gState);
	}
}
BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_on_condition)
(benchmark::State& gState)
{
	for(auto _ : gState)
	{
		on_condition(gState);
	}
}

BENCHMARK_REGISTER_F(filtering_fixture, trivial_filtering_int)->Unit(benchmark::kMicrosecond)->DenseRange(0, 3);
BENCHMARK_REGISTER_F(filtering_fixture, trivial_filtering_char_float)->Unit(benchmark::kMicrosecond)->DenseRange(0, 3);
BENCHMARK_REGISTER_F(filtering_fixture, trivial_filtering_char_int)->Unit(benchmark::kMicrosecond)->DenseRange(0, 3);
BENCHMARK_REGISTER_F(filtering_fixture, trivial_filtering_char_int_float)
  ->Unit(benchmark::kMicrosecond)
  ->DenseRange(0, 3);
BENCHMARK_REGISTER_F(filtering_fixture, trivial_filtering_float_int_char)
  ->Unit(benchmark::kMicrosecond)
  ->DenseRange(0, 3);
BENCHMARK_REGISTER_F(filtering_fixture, trivial_filtering_order_by)->Unit(benchmark::kMicrosecond)->DenseRange(0, 3);
BENCHMARK_REGISTER_F(filtering_fixture, trivial_filtering_on_condition)
  ->Unit(benchmark::kMicrosecond)
  ->DenseRange(0, 3);

#endif
#ifdef BENCHMARK_SYSTEMS

#include <random>
template <typename T, typename... Ts>
auto read_only_system = [](info_t& info, pack<T, const Ts...>) {};

template <typename T, typename... Ts>
auto write_system = [](info_t& info, pack<T, Ts...>) {};

auto get_random_entities(const psl::array<entity>& source, size_t count, std::mt19937 g)
{
	auto copy = source;
	std::shuffle(std::begin(copy), std::end(copy), g);
	copy.resize(std::min(copy.size(), count));
	return copy;
}

template <typename... Ts>
void run_system(benchmark::State& gState, state_t& state, const std::vector<entity>& count)
{
	assert(count.size() == 5);
	auto entities = state.create(count[0]);
	std::random_device rd;
	std::mt19937 g(rd());
	size_t i {1};
	(state.add_components<Ts>(get_random_entities(entities, count[i++], g)), ...);

	for(auto _ : gState)
	{
		state.tick(std::chrono::duration<float> {0.01f});
	}
}


const std::vector<std::vector<entity>> system_counts {{10'000, 300, 2'700, 1'200, 6'700},
													  {100'000, 3'000, 21'700, 10'200, 68'700},
													  {1'000'000, 3'000, 20'700, 30'200, 60'700},
													  {1'000'000, 300'000, 210'700, 300'200, 680'700}};
void trivial_read_only_seq_system(benchmark::State& gState)
{
	state_t state;
	state.declare(threading::seq, read_only_system<full, char, int, float, uint64_t>);
	run_system<char, int, float, uint64_t>(gState, state, system_counts[gState.range()]);
}

void trivial_write_seq_system(benchmark::State& gState)
{
	state_t state;
	state.declare(threading::seq, write_system<full, char, int, float, uint64_t>);
	run_system<char, int, float, uint64_t>(gState, state, system_counts[gState.range()]);
}

void trivial_read_only_par_system(benchmark::State& gState)
{
	state_t state;
	state.declare(threading::seq, read_only_system<partial, char, int, float, uint64_t>);
	run_system<char, int, float, uint64_t>(gState, state, system_counts[gState.range()]);
}

void trivial_write_par_system(benchmark::State& gState)
{
	state_t state;
	state.declare(threading::seq, write_system<partial, char, int, float, uint64_t>);
	run_system<char, int, float, uint64_t>(gState, state, system_counts[gState.range()]);
}

#include "ecs/components/camera.hpp"
#include "ecs/components/lifetime.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/velocity.hpp"
using namespace core::ecs::components;

void complex_read_only_seq_system(benchmark::State& gState)
{
	state_t state;
	state.declare(threading::seq, read_only_system<full, camera, velocity, lifetime, transform>);
	run_system<camera, velocity, lifetime, transform>(gState, state, system_counts[gState.range()]);
}

void complex_write_seq_system(benchmark::State& gState)
{
	state_t state;
	state.declare(threading::seq, write_system<full, camera, velocity, lifetime, transform>);
	run_system<camera, velocity, lifetime, transform>(gState, state, system_counts[gState.range()]);
}

void complex_read_only_par_system(benchmark::State& gState)
{
	state_t state;
	state.declare(threading::seq, read_only_system<partial, camera, velocity, lifetime, transform>);
	run_system<camera, velocity, lifetime, transform>(gState, state, system_counts[gState.range()]);
}

void complex_write_par_system(benchmark::State& gState)
{
	state_t state;
	state.declare(threading::seq, write_system<partial, camera, velocity, lifetime, transform>);
	run_system<camera, velocity, lifetime, transform>(gState, state, system_counts[gState.range()]);
}


BENCHMARK(trivial_read_only_seq_system)->DenseRange(0, 3)->Unit(benchmark::kMicrosecond);
BENCHMARK(trivial_write_seq_system)->DenseRange(0, 3)->Unit(benchmark::kMicrosecond);
BENCHMARK(trivial_read_only_par_system)->DenseRange(0, 3)->Unit(benchmark::kMicrosecond);
BENCHMARK(trivial_write_par_system)->DenseRange(0, 3)->Unit(benchmark::kMicrosecond);

BENCHMARK(complex_read_only_seq_system)->DenseRange(0, 3)->Unit(benchmark::kMicrosecond);
BENCHMARK(complex_write_seq_system)->DenseRange(0, 3)->Unit(benchmark::kMicrosecond);
BENCHMARK(complex_read_only_par_system)->DenseRange(0, 3)->Unit(benchmark::kMicrosecond);
BENCHMARK(complex_write_par_system)->DenseRange(0, 3)->Unit(benchmark::kMicrosecond);

#endif