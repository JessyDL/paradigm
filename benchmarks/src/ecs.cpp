#include "psl/ecs/on_condition.hpp"
#include "psl/ecs/order_by.hpp"
#include "psl/ecs/state.hpp"
#include "psl/math/math.hpp"
#include <benchmark/benchmark.h>
#include <random>

#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/lifetime.hpp"
#include "core/ecs/components/transform.hpp"
#include "core/ecs/components/velocity.hpp"
using namespace psl;
using namespace psl::ecs;
using namespace core::ecs::components;

#define BENCHMARK_ENTITY_CREATION
#define BENCHMARK_COMPONENT_CREATION
#define BENCHMARK_FILTERING
#define BENCHMARK_SYSTEMS


#define DEFINE_AND_REGISTER_BENCHMARK(fixture, name, ...)                                                              \
	BENCHMARK_TEMPLATE_DEFINE_F(fixture, name, __VA_ARGS__)(benchmark::State & gState) {                               \
		run_benchmark(gState);                                                                                         \
	}                                                                                                                  \
	BENCHMARK_REGISTER_F(fixture, name)->Unit(benchmark::kMicrosecond)->Threads(1)

class SharedResourceManager {
  public:
	static SharedResourceManager& getInstance() {
		static SharedResourceManager instance;
		return instance;
	}

	psl::ecs::state_t& getResource() {
		return resource;
	}

  private:
	SharedResourceManager() : resource(0, 256 * 1024 * 1024) {}
	psl::ecs::state_t resource;
};


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

template <typename T = size_t>
constexpr T get_range(benchmark::State const& gState, size_t pos = 0) {
	static_assert(std::is_unsigned_v<T>, "T must be an unsigned type");

	auto range = gState.range(pos);
	if(range < 0) {
		throw std::invalid_argument("Range must be positive");
	}
	if(static_cast<uint64_t>(range) > static_cast<uint64_t>(std::numeric_limits<T>::max())) {
		throw std::out_of_range("Range exceeds maximum limit for unsigned int");
	}

	return static_cast<T>(range);
}

#ifdef BENCHMARK_ENTITY_CREATION
template <bool Destruction, bool Shuffle>
void entity_creation_fn(benchmark::State& gState) {
	auto const eCount = get_range<psl::ecs::entity_t::size_type>(gState, 0);

	ecs::state_t state;
	for(auto _ : gState) {
		gState.PauseTiming();
		state.clear();
		gState.ResumeTiming();
		auto ents = state.create(eCount);
		if constexpr(Destruction) {
			ents.erase(std::next(std::begin(ents), eCount >> 1), std::end(ents));

			if constexpr(Shuffle) {
		gState.PauseTiming();
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(std::begin(ents), std::end(ents), g);
		gState.ResumeTiming();
			}

		state.destroy(ents);
			state.create(eCount >> 1);
		}
	}
}

void entity_creation(benchmark::State& gState) {
	entity_creation_fn<false, false>(gState);
}

void entity_creation_with_destruction(benchmark::State& gState) {
	entity_creation_fn<true, false>(gState);
	}

void entity_creation_with_destruction_shuffle(benchmark::State& gState) {
	entity_creation_fn<true, true>(gState);
}

BENCHMARK(entity_creation)->RangeMultiplier(10)->Range(100, 1'000'000)->Unit(benchmark::kMicrosecond);
BENCHMARK(entity_creation_with_destruction)->RangeMultiplier(10)->Range(100, 1'000'000)->Unit(benchmark::kMicrosecond);
BENCHMARK(entity_creation_with_destruction_shuffle)
  ->RangeMultiplier(10)
  ->Range(100, 1'000'000)
  ->Unit(benchmark::kMicrosecond);
#endif

#ifdef BENCHMARK_COMPONENT_CREATION

struct some_type_t {
	some_type_t() : value(0) {}
	some_type_t(uint64_t v) : value(v) {}
	some_type_t(const some_type_t& other) : value(other.value) {}
	some_type_t(some_type_t&& other) noexcept : value(other.value) {
		other.value = 0;
	}
	some_type_t& operator=(const some_type_t& other) {
		if(this != &other) {
			value = other.value;
		}
		return *this;
	}
	some_type_t& operator=(some_type_t&& other) noexcept {
		if(this != &other) {
			value		= other.value;
			other.value = 0;
		}
		return *this;
	}
	~some_type_t() {};
	uint64_t value {0};
};

struct some_type2_t : public some_type_t {};
struct some_type3_t : public some_type_t {};
struct some_type4_t : public some_type_t {};
struct some_type5_t : public some_type_t {};

void component_creation_baseline(benchmark::State& gState) {
	auto eCount = get_range<psl::ecs::entity_t::size_type>(gState, 0);
	auto cCount = gState.range(1);


	for(auto _ : gState) {
		std::vector<int> vec0;
		std::vector<float> vec1;
		std::vector<char> vec2;
		std::vector<bool> vec3;
		std::vector<uint64_t> vec4;
		if(cCount >= 5)
			benchmark::DoNotOptimize([&]() {
				vec4.resize(eCount);
				benchmark::DoNotOptimize(vec4);
				return vec4.data();
			}());
		if(cCount >= 4)
			benchmark::DoNotOptimize([&]() {
				vec3.resize(eCount);
				benchmark::DoNotOptimize(vec3);
				return vec3.back();
			}());
		if(cCount >= 3)
			benchmark::DoNotOptimize([&]() {
				vec2.resize(eCount);
				benchmark::DoNotOptimize(vec2);
				return vec2.data();
			}());
		if(cCount >= 2)
			benchmark::DoNotOptimize([&]() {
				vec1.resize(eCount);
				benchmark::DoNotOptimize(vec1);
				return vec1.data();
			}());

		benchmark::DoNotOptimize([&]() {
			vec0.resize(eCount);
			benchmark::DoNotOptimize(vec0);
			return vec0.data();
		}());
	}
}

void component_creation_no_mod(benchmark::State& gState) {
	auto eCount = get_range<psl::ecs::entity_t::size_type>(gState, 0);
	auto cCount = gState.range(1);
	psl::array<std::unique_ptr<psl::ecs::details::component_container_t>> containers {};

	ecs::state_t state;
	auto entities = state.create(eCount);
	for(auto _ : gState) {
		gState.PauseTiming();
		containers.clear();
		containers.emplace_back(psl::ecs::details::instantiate_component_container<int>());
		if(cCount >= 2)
			containers.emplace_back(psl::ecs::details::instantiate_component_container<float>());
		if(cCount >= 3)
			containers.emplace_back(psl::ecs::details::instantiate_component_container<char>());
		if(cCount >= 4)
			containers.emplace_back(psl::ecs::details::instantiate_component_container<bool>());
		if(cCount >= 5)
			containers.emplace_back(psl::ecs::details::instantiate_component_container<uint64_t>());
		gState.ResumeTiming();
		for(auto i = 0; i < cCount; ++i) {
			containers[i]->add(entities);
		}
	}
}

void component_creation(benchmark::State& gState) {
	auto eCount = get_range<psl::ecs::entity_t::size_type>(gState, 0);
	auto cCount = gState.range(1);
	ecs::state_t state;
	auto entities = state.create(eCount);

	for(auto _ : gState) {
		gState.PauseTiming();
		state.clear();
		gState.ResumeTiming();
		if(cCount >= 5)
			state.add_components<uint64_t>(entities);
		if(cCount >= 4)
			state.add_components<bool>(entities);
		if(cCount >= 3)
			state.add_components<char>(entities);
		if(cCount >= 2)
			state.add_components<float>(entities);

		state.add_components<int>(entities);
	}
}

void complex_component_creation_baseline(benchmark::State& gState) {
	auto eCount = get_range<psl::ecs::entity_t::size_type>(gState, 0);
	auto cCount = gState.range(1);
	for(auto _ : gState) {
		std::vector<some_type_t> vec0;
		std::vector<some_type2_t> vec1;
		std::vector<some_type3_t> vec2;
		std::vector<some_type4_t> vec3;
		std::vector<some_type5_t> vec4;

		if(cCount >= 5)
			benchmark::DoNotOptimize([&]() {
				vec4.resize(eCount);
				return vec4.data();
			}());
		if(cCount >= 4)
			benchmark::DoNotOptimize([&]() {
				vec3.resize(eCount);
				return vec3.data();
			}());
		if(cCount >= 3)
			benchmark::DoNotOptimize([&]() {
				vec2.resize(eCount);
				return vec2.data();
			}());
		if(cCount >= 2)
			benchmark::DoNotOptimize([&]() {
				vec1.resize(eCount);
				return vec1.data();
			}());

		benchmark::DoNotOptimize([&]() {
			vec0.resize(eCount);
			return vec0.data();
		}());
	}
}

void complex_component_creation_no_mod(benchmark::State& gState) {
	auto eCount = get_range<psl::ecs::entity_t::size_type>(gState, 0);
	auto cCount = gState.range(1);
	psl::array<std::unique_ptr<psl::ecs::details::component_container_t>> containers {};

	ecs::state_t state;
	auto entities = state.create(eCount);
	for(auto _ : gState) {
		gState.PauseTiming();
		containers.clear();
		containers.emplace_back(psl::ecs::details::instantiate_component_container<some_type_t>());
		if(cCount >= 2)
			containers.emplace_back(psl::ecs::details::instantiate_component_container<some_type2_t>());
		if(cCount >= 3)
			containers.emplace_back(psl::ecs::details::instantiate_component_container<some_type3_t>());
		if(cCount >= 4)
			containers.emplace_back(psl::ecs::details::instantiate_component_container<some_type4_t>());
		if(cCount >= 5)
			containers.emplace_back(psl::ecs::details::instantiate_component_container<some_type5_t>());
		gState.ResumeTiming();
		for(auto i = 0; i < cCount; ++i) {
			containers[i]->add(entities);
		}
	}
}

void complex_component_creation(benchmark::State& gState) {
	auto eCount = get_range<psl::ecs::entity_t::size_type>(gState, 0);
	auto cCount = gState.range(1);
	ecs::state_t state;
	auto entities = state.create(eCount);

	for(auto _ : gState) {
		gState.PauseTiming();
		state.clear();
		gState.ResumeTiming();
		if(cCount >= 5)
			state.add_components<some_type5_t>(entities);
		if(cCount >= 4)
			state.add_components<some_type4_t>(entities);
		if(cCount >= 3)
			state.add_components<some_type3_t>(entities);
		if(cCount >= 2)
			state.add_components<some_type2_t>(entities);

		state.add_components<some_type_t>(entities);
	}
}

void component_creation_args(benchmark::internal::Benchmark* b) {
	for(int j = 1; j <= 5; ++j) {
		for(int i = 2; i <= 6; ++i) {
			b->ArgPair((int64_t)pow(10, i), j);
		}
	}
}

BENCHMARK(component_creation)->Apply(component_creation_args)->Unit(benchmark::kMicrosecond);
// BENCHMARK(component_creation_no_mod)->Apply(component_creation_args)->Unit(benchmark::kMicrosecond);
// BENCHMARK(component_creation_baseline)->Apply(component_creation_args)->Unit(benchmark::kMicrosecond);
BENCHMARK(complex_component_creation)->Apply(component_creation_args)->Unit(benchmark::kMicrosecond);
// BENCHMARK(complex_component_creation_no_mod)->Apply(component_creation_args)->Unit(benchmark::kMicrosecond);
// BENCHMARK(complex_component_creation_baseline)->Apply(component_creation_args)->Unit(benchmark::kMicrosecond);
#endif

template <template <typename...> typename PackType>
class idiomatic_system_usage : public ::benchmark::Fixture {
  public:
	void SetUp(const ::benchmark::State& gState) override {
		auto& state				= SharedResourceManager::getInstance().getResource();
		auto const entity_count = get_range<psl::ecs::entity_t::size_type>(gState, 0);

		auto base_entities = state.create<int, transform, camera>(entity_count);
		auto to_erase	   = base_entities;

		auto index				  = 0;
		auto const max_iterations = 20;
		for(auto iteration = 0; iteration < max_iterations; ++iteration) {
			// shuffle the entities to remove random entries.
			// additionally we do this at the start as the end of the loop will
			// erase and create new entities which we'll want to properly shuffle
			// again.
			std::random_device rd;
			std::mt19937 g(rd());
			std::shuffle(std::begin(to_erase), std::end(to_erase), g);

			auto begin = std::begin(to_erase);
			auto end   = std::next(begin, entity_count / max_iterations);
			psl::array<psl::ecs::entity_t> transforms_to_remove {};
			psl::array<psl::ecs::entity_t> cameras_to_remove {};
			psl::array<psl::ecs::entity_t> ints_to_remove {};
			psl::array<psl::ecs::entity_t> entities_to_remove {};
			for(auto i = 0; begin != end; ++begin, ++i) {
				// some prime numbers
				if(i % 7 <= 3) {
					transforms_to_remove.emplace_back(*begin);
				}
				if(i % 11 <= 5) {
					cameras_to_remove.emplace_back(*begin);
				}
				if(i % 17 <= 5) {
					entities_to_remove.emplace_back(*begin);
				}
				if(i % 19 <= 7) {
					ints_to_remove.emplace_back(*begin);
				}
			}

			// most entities will be created & destroyed during the system calls, so we do the creation here
			// before we tick the state as that's how the lifetime of the entities is best reflected.

			state.remove_components<transform>(transforms_to_remove);
			state.remove_components<camera>(cameras_to_remove);
			state.remove_components<int>(ints_to_remove);
			state.destroy(entities_to_remove);

			auto new_entities = state.create<int, transform, camera>(entity_count / max_iterations);
			for(entity_t::size_type i = 0; i < entity_count / max_iterations; ++i) {
				to_erase[i] = new_entities[i];
			}

			// ticking will promote the entities to the next lifetime stage, erased entities will be removed
			// and new entities will become settled.
			state.tick(std::chrono::duration<float> {1.f});
		}

		state.declare(psl::ecs::threading::seq, &idiomatic_system_usage::system, this);
	}

	void system(info_t& buffer, PackType<entity_t, int, const transform, const camera> pack) {
		for(auto [e, i, transf, cam] : pack) {
			i += e.value();
		}
	}
	void TearDown(const ::benchmark::State& gState) override {
		SharedResourceManager::getInstance().getResource().clear(true);
	}
	void run_benchmark(benchmark::State& gState) {
		for(auto _ : gState) {
			SharedResourceManager::getInstance().getResource().tick(std::chrono::duration<float> {1.f});
		}
	}
};

DEFINE_AND_REGISTER_BENCHMARK(idiomatic_system_usage, indirect_full, pack_indirect_full_t)
  ->RangeMultiplier(10)
  ->Range(1000, 1'000'000);
DEFINE_AND_REGISTER_BENCHMARK(idiomatic_system_usage, indirect_partial, pack_indirect_partial_t)
  ->RangeMultiplier(10)
  ->Range(1000, 1'000'000);
DEFINE_AND_REGISTER_BENCHMARK(idiomatic_system_usage, direct_full, pack_direct_full_t)
  ->RangeMultiplier(10)
  ->Range(1000, 1'000'000);
DEFINE_AND_REGISTER_BENCHMARK(idiomatic_system_usage, direct_partial, pack_direct_partial_t)
  ->RangeMultiplier(10)
  ->Range(1000, 1'000'000);

#ifdef BENCHMARK_FILTERING

template <typename... Ts>
class filtering_fixture : public ::benchmark::Fixture {
	const std::vector<std::vector<int>> data_constraint {{10'000, 300, 2'700, 1'200, 6'700},
														 {100'000, 3'000, 21'700, 10'200, 68'700},
														 {10'000, 300, 2'700, 3'200, 6'700},
														 {100'000, 3'000, 21'700, 30'200, 68'700}};

  public:
	void SetUp(const ::benchmark::State& gState) override {
		auto index		 = gState.range();
		const auto& data = data_constraint[psl::narrow_cast<size_t>(index)];
		auto eCount		 = data[0];
		auto char_beg	 = data[1];
		auto char_end	 = data[2];
		auto int_beg	 = data[3];
		auto int_end	 = data[4];

		auto entities = state.create(eCount);

		state.add_components<float>(
		  psl::array<entity_t> {std::begin(entities), std::next(std::begin(entities), eCount)});
		state.add_components<char>(
		  psl::array<entity_t> {std::next(std::begin(entities), char_beg), std::next(std::begin(entities), char_end)});

		state.add_components(
		  psl::array<entity_t> {std::next(std::begin(entities), int_beg), std::next(std::begin(entities), int_end)},
		  [](int& i) { i = std::rand() % 1000; });
	}

	void filter() {
		state.filter<Ts...>();
	}

	void order_by(benchmark::State& gState) {
		gState.PauseTiming();
		auto entities = state.filter<int>();
		gState.ResumeTiming();

		psl::ecs::details::order_by<std::less<int>, int>(state, std::begin(entities), std::end(entities));
	}

	void on_condition(benchmark::State& gState) {
		gState.PauseTiming();
		auto entities = state.filter<int>();
		gState.ResumeTiming();

		psl::ecs::details::on_condition<int>(
		  state, std::begin(entities), std::end(entities), [](const int& i) { return i < 500; });
	}
	ecs::state_t state;
};

BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_int, int)(benchmark::State& gState) {
	for(auto _ : gState) {
		filter();
	}
}

BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_char_float, char, float)(benchmark::State& gState) {
	for(auto _ : gState) {
		filter();
	}
}
BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_char_int, char, int)(benchmark::State& gState) {
	for(auto _ : gState) {
		filter();
	}
}
BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_char_int_float, char, int, float)
(benchmark::State& gState) {
	for(auto _ : gState) {
		filter();
	}
}
BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_float_int_char, float, int, char)
(benchmark::State& gState) {
	for(auto _ : gState) {
		filter();
	}
}

BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_order_by)
(benchmark::State& gState) {
	for(auto _ : gState) {
		order_by(gState);
	}
}
BENCHMARK_TEMPLATE_DEFINE_F(filtering_fixture, trivial_filtering_on_condition)
(benchmark::State& gState) {
	for(auto _ : gState) {
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


template <template <typename...> typename PackType, typename... Ts>
class basic_system_usage : public ::benchmark::Fixture {
	const std::vector<std::vector<entity_t::size_type>> system_counts {{10'000, 300, 2'700, 1'200, 6'700},
																	   {100'000, 3'000, 21'700, 10'200, 68'700},
																	   {1'000'000, 3'000, 20'700, 30'200, 60'700},
																	   {1'000'000, 300'000, 210'700, 300'200, 680'700}};

  public:
	void SetUp(const ::benchmark::State& gState) override {
		auto counts_entry = system_counts[get_range<size_t>(gState)];
		auto eCount		  = counts_entry[0];
		auto entities	  = SharedResourceManager::getInstance().getResource().create(eCount);

		auto create_random_entity_array = [](const psl::array<entity_t>& source, size_t count, std::mt19937 g) {
			auto copy = source;
			std::shuffle(std::begin(copy), std::end(copy), g);
			copy.resize(std::min(copy.size(), count));
			return copy;
		};

		std::random_device rd;
		std::mt19937 g(rd());
		size_t i {1};
		(SharedResourceManager::getInstance().getResource().add_components<std::remove_const_t<Ts>>(
		   create_random_entity_array(entities, counts_entry[i++], g)),
		 ...);

		SharedResourceManager::getInstance().getResource().declare(threading::seq,
																   [](info_t& info, PackType<Ts...> pack) {});
	}
	void TearDown(const ::benchmark::State& gState) override {
		SharedResourceManager::getInstance().getResource().clear(true);
	}
	void run_benchmark(benchmark::State& gState) {
		for(auto _ : gState) {
			SharedResourceManager::getInstance().getResource().tick(std::chrono::duration<float> {1.f});
		}
	}
};


	#define CONST_TRIVIAL_COMPONENT_TYPES const char, const int, const float, const uint64_t
	#define TRIVIAL_COMPONENT_TYPES const char, const int, const float, const uint64_t
	#define CONST_COMPLEX_COMPONENT_TYPES const camera, const velocity, const lifetime, const transform
	#define COMPLEX_COMPONENT_TYPES camera, velocity, lifetime, transform

	#define DEFINE_ALL_PACK_VARIANTS(fixture, base_name, ...)                                                          \
		DEFINE_AND_REGISTER_BENCHMARK(fixture, base_name##_seq_indirect_system, pack_indirect_full_t, __VA_ARGS__)     \
		  ->DenseRange(0, 3);                                                                                          \
		DEFINE_AND_REGISTER_BENCHMARK(fixture, base_name##_seq_direct_system, pack_direct_full_t, __VA_ARGS__)         \
		  ->DenseRange(0, 3);                                                                                          \
		DEFINE_AND_REGISTER_BENCHMARK(fixture, base_name##_par_indirect_system, pack_indirect_partial_t, __VA_ARGS__)  \
		  ->DenseRange(0, 3);                                                                                          \
		DEFINE_AND_REGISTER_BENCHMARK(fixture, base_name##_par_direct_system, pack_direct_partial_t, __VA_ARGS__)      \
		  ->DenseRange(0, 3)

DEFINE_ALL_PACK_VARIANTS(basic_system_usage, trivial_read_only, CONST_TRIVIAL_COMPONENT_TYPES);
DEFINE_ALL_PACK_VARIANTS(basic_system_usage, trivial_write, TRIVIAL_COMPONENT_TYPES);
DEFINE_ALL_PACK_VARIANTS(basic_system_usage, complex_read_only, CONST_COMPLEX_COMPONENT_TYPES);
DEFINE_ALL_PACK_VARIANTS(basic_system_usage, complex_write, COMPLEX_COMPONENT_TYPES);

	#undef DEFINE_ALL_PACK_VARIANTS
	#undef CONST_TRIVIAL_COMPONENT_TYPES
	#undef TRIVIAL_COMPONENT_TYPES
	#undef CONST_COMPLEX_COMPONENT_TYPES
	#undef COMPLEX_COMPONENT_TYPES
#endif
