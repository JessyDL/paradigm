#include "psl/ecs/state.h"
#include <numeric>
#include "psl/unique_ptr.h"
//#include "psl/async/algorithm.h"
using namespace psl::ecs;

using psl::ecs::details::component_key_t;
using psl::ecs::details::entity_info;

constexpr size_t min_thread_entities = 1;


state::state(size_t workers) : m_Scheduler(new psl::async::scheduler((workers == 0) ? 1 : std::optional{workers})) {}


std::vector<std::vector<details::dependency_pack>> slice(std::vector<details::dependency_pack>& source,
														 size_t workers = std::numeric_limits<size_t>::max())
{
	std::vector<std::vector<details::dependency_pack>> packs;

	if(source.size() == 0) return packs;

	auto [smallest_batch, largest_batch] =
		std::minmax_element(std::begin(source), std::end(source),
							[](const auto& lhs, const auto& rhs) { return lhs.entities() < rhs.entities(); });
	workers			 = std::min<size_t>(workers, std::thread::hardware_concurrency());
	auto max_workers = std::max<size_t>(1u, std::min(workers, largest_batch->entities() % min_thread_entities));

	// To guard having systems run with concurrent packs that have no data in them.
	// Doing so would seem counter-intuitive to users
	while((float)smallest_batch->entities() / (float)max_workers < 1.0f && max_workers > 1)
	{
		--max_workers;
	}
	workers = max_workers;

	packs.resize(workers);
	for(auto& dep_pack : source)
	{
		if(dep_pack.allow_partial())
		{
			auto batch_size = dep_pack.entities() / workers;
			size_t processed{0};
			for(auto i = 0; i < workers - 1; ++i)
			{
				packs[i].emplace_back(dep_pack.slice(processed, processed + batch_size));
				processed += batch_size;
			}
			packs[packs.size() - 1].emplace_back(dep_pack.slice(processed, dep_pack.entities()));
		}
		else // if packs cannot be split, then emplace the 'full' data
		{
			for(auto i = 0; i < workers; ++i) packs[i].emplace_back(dep_pack);
		}
	}
	return packs;
}

std::shared_ptr<std::vector<std::vector<details::dependency_pack>>>
shared_slice(std::vector<details::dependency_pack>& source, size_t workers = std::numeric_limits<size_t>::max())
{
	std::shared_ptr<std::vector<std::vector<details::dependency_pack>>> shared_packs =
		std::make_shared<std::vector<std::vector<details::dependency_pack>>>();

	if(source.size() == 0) return shared_packs;

	auto [smallest_batch, largest_batch] =
		std::minmax_element(std::begin(source), std::end(source),
							[](const auto& lhs, const auto& rhs) { return lhs.entities() < rhs.entities(); });
	workers			 = std::min<size_t>(workers, std::thread::hardware_concurrency());
	auto max_workers = std::max<size_t>(1u, std::min(workers, largest_batch->entities() % min_thread_entities));

	// To guard having systems run with concurrent packs that have no data in them.
	// Doing so would seem counter-intuitive to users
	while((float)smallest_batch->entities() / (float)max_workers < 1.0f && max_workers > 1)
	{
		--max_workers;
	}
	workers = max_workers;

	shared_packs->resize(workers);
	for(auto& dep_pack : source)
	{
		if(dep_pack.allow_partial())
		{
			auto batch_size = dep_pack.entities() / workers;
			size_t processed{0};
			for(auto i = 0; i < workers - 1; ++i)
			{
				(*shared_packs)[i].emplace_back(dep_pack.slice(processed, processed + batch_size));
				processed += batch_size;
			}
			(*shared_packs)[shared_packs->size() - 1].emplace_back(dep_pack.slice(processed, dep_pack.entities()));
		}
		else // if packs cannot be split, then emplace the 'full' data
		{
			for(auto i = 0; i < workers; ++i) (*shared_packs)[i].emplace_back(dep_pack);
		}
	}
	return shared_packs;
}

psl::array<psl::array<entity>> slice(psl::array_view<entity> source,
									 size_t workers = std::numeric_limits<size_t>::max())
{
	psl::array<psl::array<entity>> res;

	workers = std::min<size_t>(workers, std::thread::hardware_concurrency());
	workers = std::max<size_t>(1u, std::min(workers, source.size() % min_thread_entities));

	res.reserve(workers);

	auto current = std::begin(source);
	for(auto i = 0u; i < workers - 1; ++i)
	{
		auto next = std::next(current, source.size() / workers);

		res.emplace_back(current, next);
		current = next;
	}

	res.emplace_back(current, std::end(source));
	return res;
}



void state::prepare_system(std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
						   std::uintptr_t cache_offset, details::system_information& information)
{
	std::function<void(state&, std::vector<details::dependency_pack>)> write_data =
		[](state& state, std::vector<details::dependency_pack> dep_packs) {
			for(const auto& dep_pack : dep_packs)
			{
				for(auto& binding : dep_pack.m_RWBindings)
				{
					const size_t size   = dep_pack.m_Sizes.at(binding.first);
					std::uintptr_t data = (std::uintptr_t)binding.second.data();
					state.set(dep_pack.m_Entities, binding.first, (void*)data);
				}
			}
		};

	//{
	//	using namespace ::async;

	//	auto shared_pack =
	//		std::make_shared<std::vector<details::dependency_pack>>(std::move(information.create_pack()));
	//	auto dep_pack_it = std::begin(*shared_pack);
	//	// parallel_for(*shared_pack, chain)
	//	for(auto& dep_pack : *shared_pack)
	//	{
	//		// 1) do an initial filtering and split the results in N packets
	//		auto task_initial_filter = [this](details::dependency_pack dep_pack) -> std::tuple<details::dependency_pack, psl::array<entity>> {
	//			return std::tuple{dep_pack, initial_filter(dep_pack)};
	//		};

	//		// 2) for N workers, give them a packet each to work with and filter those packets.
	//		auto task_filter = parallel_n(
	//			[this, min_pack_size = min_thread_entities](invocation invocation, details::dependency_pack dep_pack,
	//														psl::array_view<entity> entities) {
	//				auto remainder = min_pack_size - (entities.size() % min_pack_size);
	//				if(min_pack_size == 1) remainder = 0;
	//				auto max_workers = invocation.count;
	//				max_workers		 = std::max<size_t>(
	//					 1, std::min<size_t>(max_workers, (entities.size() + remainder) / min_pack_size));

	//				if(invocation.index >= max_workers) return std::tuple{dep_pack, psl::array<entity>{}};
	//				auto count = entities.size() / max_workers;

	//				auto begin = (count)*invocation.index;
	//				auto end   = count * (invocation.index + 1);

	//				if(invocation.index == max_workers - 1) end = entities.size();
	//				count = end - begin;

	//				auto result = psl::array<entity>{};

	//				result.insert(std::end(result), std::next(std::begin(entities), begin),
	//							  std::next(std::begin(entities), end));

	//				auto begin_it = std::begin(result);
	//				auto end_it   = std::end(result);

	//				std::sort(begin_it, end_it);
	//				end_it = filter(begin_it, end_it, dep_pack);
	//				result.erase(end_it, std::end(result));

	//				return std::tuple{dep_pack, result};
	//			},
	//			8);

	//		// 3) we merge the results into one giant array which we'll evenly split amongst the workers
	//		auto task_merge_filter = [](details::dependency_pack dep_pack, std::vector<psl::array<entity>> entity_packs) {
	//			psl::array<entity> res;
	//			for(auto& entities : entity_packs)
	//			{
	//				auto middle = std::size(res);
	//				res.insert(std::end(res), std::begin(entities), std::end(entities));
	//				std::inplace_merge(std::begin(res), std::next(std::begin(res), middle), std::end(res));
	//			}
	//			return std::tuple{dep_pack, res};
	//		};

	//		// 4) we split the entities and then apply the order_by operation
	//		auto task_order_entities = parallel_n(
	//			then(
	//				[this, min_pack_size = min_thread_entities](invocation invocation,
	//															details::dependency_pack dep_pack,
	//															psl::array_view<entity> merged_entities) {
	//					auto remainder = min_pack_size - (merged_entities.size() % min_pack_size);
	//					if(min_pack_size == 1) remainder = 0;
	//					auto max_workers =
	//						std::max<size_t>(1, std::min<size_t>(invocation.count,
	//															 (merged_entities.size() + remainder) / min_pack_size));
	//					if(invocation.index >= max_workers)
	//						return std::tuple<details::dependency_pack, size_t, psl::array<entity>, size_t>{
	//							dep_pack, 0, {}, merged_entities.size()};

	//					auto count = merged_entities.size() / max_workers;
	//					auto begin = (count)*invocation.index;
	//					auto end   = count * (invocation.index + 1);
	//					if(invocation.index == max_workers - 1) end = merged_entities.size();
	//					count = end - begin;

	//					psl::array<entity> entities(std::next(std::begin(merged_entities), begin),
	//												std::next(std::begin(merged_entities), end));

	//					std::invoke(dep_pack.orderby, std::begin(entities), std::end(entities), *this);
	//					return std::tuple{dep_pack, begin, entities, merged_entities.size()};
	//				},

	//				[this, cache_offset](details::dependency_pack dep_pack, size_t offset,
	//									 psl::array_view<entity> entities, size_t total) {
	//					if(entities.size() == 0) return;
	//					auto cache_location = cache_offset;
	//					/*for(const auto& write_size : write_sizes)
	//					{
	//						cache_location += write_size.get();
	//					}*/

	//					void* cache = (void*)(cache_location + (sizeof(entity) * offset));
	//					std::memcpy(cache, entities.data(), sizeof(entity) * entities.size());
	//					cache = (void*)(cache_location + (sizeof(entity) * total));

	//					for(const auto& binding : dep_pack.m_RBindings)
	//					{
	//						auto written = prepare_data(
	//							entities, (void*)((std::uintptr_t)cache + (dep_pack.size_of(binding.first) * offset)),
	//							binding.first);

	//						assert_debug_break(written == entities.size() * dep_pack.size_of(binding.first));
	//						cache = (void*)((std::uintptr_t)cache + (dep_pack.size_of(binding.first) * total));
	//					}
	//					for(const auto& binding : dep_pack.m_RWBindings)
	//					{
	//						auto written = prepare_data(
	//							entities, (void*)((std::uintptr_t)cache + (dep_pack.size_of(binding.first) * offset)),
	//							binding.first);
	//						assert_debug_break(written == entities.size() * dep_pack.size_of(binding.first));
	//						cache = (void*)((std::uintptr_t)cache + (dep_pack.size_of(binding.first) * total));
	//					}
	//				}),
	//			8);

	//		auto chain = then(task_initial_filter, [](auto... values) {});

	//		compute(chain, dep_pack);
	//	}
	//}

	////{
	////	auto shared_pack =
	////		std::make_shared<std::vector<details::dependency_pack>>(std::move(information.create_pack()));
	////	auto dep_pack_it = std::begin(*shared_pack);
	////	for(auto& dep_pack : *shared_pack)
	////	{
	////		async::token merged_entities_token = m_Scheduler->proxy();
	////		// 1) do an initial filtering and split the results in N packets
	////		auto [prepare_token, prepare_future] = m_Scheduler->schedule<std::shared_future>(
	////			[this, dep_pack]() -> psl::array<entity> { return initial_filter(dep_pack); });

	////		psl::array<std::shared_future<psl::array<entity>>> filtered_ends;
	////		// 2) for N workers, give them a packet each to work with and filter those packets.
	////		for(size_t i = 0; i < m_Scheduler->workers(); ++i)
	////		{
	////			auto [filter_token, filter_future] = m_Scheduler->schedule<std::shared_future>([
	////				this, prepare_future = prepare_future, dep_pack, i, min_pack_size = min_thread_entities,
	////				max_workers = m_Scheduler->workers()
	////			]() mutable noexcept {
	////				const psl::array<entity>& all_entities = prepare_future.get();
	////				auto remainder						   = min_pack_size - (all_entities.size() % min_pack_size);
	////				max_workers							   = std::max<size_t>(
	////					   1, std::min<size_t>(max_workers, (all_entities.size() + remainder) / min_pack_size));
	////				if(i >= max_workers) return psl::array<entity>{};

	////				auto count = all_entities.size() / max_workers;
	////				auto begin = (count)*i;
	////				auto end   = count * (i + 1);
	////				if(i == max_workers - 1) end = all_entities.size();
	////				count = end - begin;

	////				psl::array<entity> entities{std::next(std::begin(all_entities), begin),
	////											std::next(std::begin(all_entities), end)};
	////				std::sort(std::begin(entities), std::end(entities));
	////				entities.erase(filter(std::begin(entities), std::end(entities), dep_pack), std::end(entities));
	////				return entities;
	////			});

	////			filter_token.after(prepare_token);
	////			merged_entities_token.after(filter_token);
	////			filtered_ends.emplace_back(filter_future);
	////		}

	////		// 3) we merge the results into one giant array which we'll evenly split amongst the workers
	////		auto merged_entities_future = m_Scheduler->substitute<std::shared_future>(
	////			merged_entities_token, [filtered_entities = filtered_ends]() mutable -> psl::array<entity> {
	////				psl::array<entity> res;
	////				for(auto& future : filtered_entities)
	////				{
	////					auto e		= future.get();
	////					auto middle = std::size(res);
	////					res.insert(std::end(res), std::begin(e), std::end(e));
	////					std::inplace_merge(std::begin(res), std::next(std::begin(res), middle), std::end(res));
	////				}
	////				return res;
	////			});

	////		m_Scheduler->execute();

	////		auto initial = initial_filter(dep_pack);
	////		auto copy	= initial;
	////		copy.erase(filter(std::begin(copy), std::end(copy), dep_pack), std::end(copy));
	////		auto entities = filter(dep_pack);
	////		std::sort(std::begin(entities), std::end(entities));
	////		auto other_entities = merged_entities_future.get();
	////		assert_debug_break(std::equal(std::begin(entities), std::end(entities), std::begin(other_entities),
	////									  std::end(other_entities)));
	////		entities = filter(dep_pack);
	////		copy = initial;
	////		copy.erase(filter(std::begin(copy), std::end(copy), dep_pack), std::end(copy));
	////	}
	////}

	//if(false)
	//{
	//	// we make a reference counted vector of dependency packs that will be used by subsequent tasks
	//	auto shared_pack =
	//		std::make_shared<std::vector<details::dependency_pack>>(std::move(information.create_pack()));

	//	psl::array<std::shared_future<std::uintptr_t>> write_sizes;
	//	psl::array<async::token> write_sizes_tokens{};
	//	psl::array<async::token> bindings_tasks{};
	//	psl::array<async::token> write_to_state_tokens{};
	//	auto dep_pack_it = std::begin(*shared_pack);
	//	for(auto& dep_pack : *shared_pack)
	//	{
	//		async::token merged_entities_token = m_Scheduler->proxy();
	//		// 1) do an initial filtering and split the results in N packets
	//		auto [prepare_token, prepare_future] = m_Scheduler->schedule<std::shared_future>(
	//			[this, dep_pack]() -> psl::array<entity> { return initial_filter(dep_pack); });

	//		psl::array<std::future<psl::array<entity>>> filtered_ends;
	//		// 2) for N workers, give them a packet each to work with and filter those packets.
	//		for(size_t i = 0; i < m_Scheduler->workers(); ++i)
	//		{
	//			auto [filter_token, filter_future] = m_Scheduler->schedule([
	//				this, prepare_future = prepare_future, dep_pack, i, min_pack_size = min_thread_entities,
	//				max_workers = m_Scheduler->workers()
	//			]() mutable noexcept {
	//				const psl::array<entity>& all_entities = prepare_future.get();
	//				auto remainder						   = min_pack_size - (all_entities.size() % min_pack_size);
	//				max_workers							   = std::max<size_t>(
	//					   1, std::min<size_t>(max_workers, (all_entities.size() + remainder) / min_pack_size));
	//				if(i >= max_workers) return psl::array<entity>{};

	//				auto count = all_entities.size() / max_workers;
	//				auto begin = (count)*i;
	//				auto end   = count * (i + 1);
	//				if(i == max_workers - 1) end = all_entities.size();
	//				count = end - begin;

	//				psl::array<entity> entities{std::next(std::begin(all_entities), begin),
	//											std::next(std::begin(all_entities), end)};
	//				std::sort(std::begin(entities), std::end(entities));
	//				entities.erase(filter(std::begin(entities), std::end(entities), dep_pack), std::end(entities));
	//				return entities;
	//			});

	//			filter_token.after(prepare_token);
	//			merged_entities_token.after(filter_token);
	//			filtered_ends.emplace_back(std::move(filter_future));
	//		}

	//		// 3) we merge the results into one giant array which we'll evenly split amongst the workers
	//		auto merged_entities_future = m_Scheduler->substitute<std::shared_future>(
	//			merged_entities_token, [filtered_entities = std::move(filtered_ends)]() mutable -> psl::array<entity> {
	//				psl::array<entity> res;
	//				for(auto& future : filtered_entities)
	//				{
	//					auto&& e	= future.get();
	//					auto middle = std::size(res);
	//					res.insert(std::end(res), std::begin(e), std::end(e));
	//					std::inplace_merge(std::begin(res), std::next(std::begin(res), middle), std::end(res));
	//				}
	//				return res;
	//			});


	//		struct split_workload
	//		{
	//			psl::array<entity> entities{};
	//			size_t offset{0};
	//			size_t total{0};
	//		};
	//		psl::array<async::token> copy_tasks;
	//		psl::array<std::shared_future<split_workload>> split_workloads{};
	//		for(size_t i = 0; i < m_Scheduler->workers(); ++i)
	//		{
	//			// 4) we split the entities and then apply the order_by operation
	//			auto [split_token, split_future] = m_Scheduler->schedule<std::shared_future>(
	//				[this, merged_entities_future = merged_entities_future, dep_pack, i,
	//				 min_pack_size	 = min_thread_entities,
	//				 scheduler_workers = m_Scheduler->workers()]() mutable -> split_workload {
	//					// we don't edit the values, so this is not UB
	//					psl::array<entity>& merged_entities{
	//						const_cast<psl::array<entity>&>(merged_entities_future.get())};

	//					auto remainder = min_pack_size - (merged_entities.size() % min_pack_size);
	//					auto max_workers =
	//						std::max<size_t>(1, std::min<size_t>(scheduler_workers,
	//															 (merged_entities.size() + remainder) / min_pack_size));
	//					if(i >= max_workers) return {{}, merged_entities.size(), merged_entities.size()};

	//					auto count = merged_entities.size() / max_workers;
	//					auto begin = (count)*i;
	//					auto end   = count * (i + 1);
	//					if(i == max_workers - 1) end = merged_entities.size();
	//					count = end - begin;

	//					psl::array<entity> entities(std::next(std::begin(merged_entities), begin),
	//												std::next(std::begin(merged_entities), end));

	//					std::invoke(dep_pack.orderby, std::begin(entities), std::end(entities), *this);
	//					return {entities, begin, merged_entities.size()};
	//				});

	//			split_workloads.emplace_back(split_future);
	//			// 5) for N workers, copy data to the cache
	//			copy_tasks.emplace_back(m_Scheduler->schedule(
	//				[this, split_workload = split_future, cache_offset, write_sizes = write_sizes, dep_pack, i,
	//				 min_pack_size = min_thread_entities, scheduler_workers = m_Scheduler->workers()]() {
	//					// we don't edit the values, so this is not UB
	//					const auto& workload = split_workload.get();
	//					psl::array_view<entity> entities(workload.entities);
	//					auto cache_location = cache_offset;
	//					for(const auto& write_size : write_sizes)
	//					{
	//						cache_location += write_size.get();
	//					}

	//					void* cache = (void*)(cache_location + (sizeof(entity) * workload.offset));
	//					std::memcpy(cache, entities.data(), sizeof(entity) * entities.size());
	//					cache = (void*)(cache_location + (sizeof(entity) * workload.total));

	//					for(const auto& binding : dep_pack.m_RBindings)
	//					{
	//						auto written = prepare_data(
	//							entities,
	//							(void*)((std::uintptr_t)cache + (dep_pack.size_of(binding.first) * workload.offset)),
	//							binding.first);

	//						assert_debug_break(written == entities.size() * dep_pack.size_of(binding.first));
	//						cache = (void*)((std::uintptr_t)cache + (dep_pack.size_of(binding.first) * workload.total));
	//					}
	//					for(const auto& binding : dep_pack.m_RWBindings)
	//					{
	//						auto written = prepare_data(
	//							entities,
	//							(void*)((std::uintptr_t)cache + (dep_pack.size_of(binding.first) * workload.offset)),
	//							binding.first);
	//						assert_debug_break(written == entities.size() * dep_pack.size_of(binding.first));
	//						cache = (void*)((std::uintptr_t)cache + (dep_pack.size_of(binding.first) * workload.total));
	//					}
	//				}));
	//			copy_tasks[copy_tasks.size() - 1].after(split_token);
	//			split_token.after(merged_entities_token);
	//			copy_tasks[copy_tasks.size() - 1].after(write_sizes_tokens);
	//		}

	//		bindings_tasks.emplace_back(m_Scheduler->schedule([merged_entities_future = merged_entities_future,
	//														   dep_pack_it, cache_offset, write_sizes = write_sizes]() {
	//			auto count = merged_entities_future.get().size();

	//			auto cache_location = cache_offset;
	//			for(const auto& write_size : write_sizes) cache_location += write_size.get();
	//			auto& dep_pack		= *dep_pack_it;
	//			dep_pack.m_Entities = psl::array_view<entity>((entity*)cache_location,
	//														  (entity*)(cache_location + (sizeof(entity) * count)));

	//			cache_location += (sizeof(entity) * count);

	//			for(auto& binding : dep_pack.m_RBindings)
	//			{
	//				auto write_size = dep_pack.size_of(binding.first) * count;
	//				binding.second  = psl::array_view<std::uintptr_t>((std::uintptr_t*)cache_location,
	//																  (std::uintptr_t*)(cache_location + write_size));
	//				cache_location += write_size;
	//			}
	//			for(auto& binding : dep_pack.m_RWBindings)
	//			{
	//				auto write_size = dep_pack.size_of(binding.first) * count;
	//				binding.second  = psl::array_view<std::uintptr_t>((std::uintptr_t*)cache_location,
	//																  (std::uintptr_t*)(cache_location + write_size));
	//				cache_location += write_size;
	//			}
	//		}));

	//		bindings_tasks[bindings_tasks.size() - 1].after(copy_tasks);

	//		auto [write_size_token, write_size_future] = m_Scheduler->schedule<std::shared_future>(
	//			[merged_entities_future = merged_entities_future, dep_pack, cache_offset]() {
	//				auto count = merged_entities_future.get().size();
	//				return (sizeof(entity) + dep_pack.size_per_element()) * count;
	//			});
	//		write_size_token.after(bindings_tasks);
	//		write_sizes.emplace_back(write_size_future);
	//		write_sizes_tokens.emplace_back(write_size_token);


	//		for(size_t i = 0; i < m_Scheduler->workers(); ++i)
	//		{
	//			// copy from cache to the state
	//			write_to_state_tokens.emplace_back(m_Scheduler->schedule(
	//				[this, i, split_workloads = split_workloads, dep_pack_it, min_pack_size = min_thread_entities,
	//				 max_workers = m_Scheduler->workers()]() mutable -> void {
	//					const auto& workload = split_workloads[i].get();

	//					psl::array_view<entity> entities(workload.entities);

	//					const auto& dep_pack = *dep_pack_it;
	//					for(auto& binding : dep_pack.m_RWBindings)
	//					{
	//						const size_t size   = dep_pack.m_Sizes.at(binding.first);
	//						std::uintptr_t data = (std::uintptr_t)binding.second.data() + (workload.offset * size);
	//						auto written		= set(entities, binding.first, (void*)data);
	//						assert_debug_break(written == entities.size() * size);
	//					}
	//				}));
	//		}

	//		dep_pack_it = std::next(dep_pack_it);
	//	}

	//	psl::array<async::token> system_tasks;

	//	// fire one system task or N system tasks
	//	if(std::any_of(std::begin(*shared_pack), std::end(*shared_pack),
	//				   [](const auto& dep_pack) { return dep_pack.allow_partial(); }) &&
	//	   information.threading() == threading::par)
	//	{
	//		auto [multi_token, multi_pack] =
	//			m_Scheduler->schedule<std::shared_future>([shared_pack, max_workers = m_Scheduler->workers()]() {
	//				return shared_slice(*shared_pack, max_workers);
	//			});
	//		for(size_t i = 0; i < m_Scheduler->workers(); ++i)
	//		{
	//			info_buffer.emplace_back(new info(*this, dTime, rTime));
	//			system_tasks.emplace_back(
	//				m_Scheduler->schedule([& info = *info_buffer[info_buffer.size() - 1], &information, multi_pack, i] {
	//					auto shared_multi_pack = multi_pack.get();
	//					if(i >= shared_multi_pack->size()) return;
	//					std::invoke(information, info, (*shared_multi_pack)[i]);
	//				}));
	//		}
	//		multi_token.after(bindings_tasks);
	//		for(auto& system_task : system_tasks) system_task.after(multi_token);
	//		for(auto& write_to_state : write_to_state_tokens) write_to_state.after(system_tasks);
	//	}
	//	else
	//	{
	//		info_buffer.emplace_back(new info(*this, dTime, rTime));
	//		system_tasks.emplace_back(
	//			m_Scheduler->schedule([& info = *info_buffer[info_buffer.size() - 1], &information, shared_pack] {
	//				std::invoke(information, info, *shared_pack);
	//			}));

	//		for(auto& system_task : system_tasks) system_task.after(bindings_tasks);

	//		for(auto& write_to_state : write_to_state_tokens) write_to_state.after(system_tasks);
	//	}


	//	m_Scheduler->execute();
	//}
	//else
	//{
		auto pack		 = information.create_pack();
		bool has_partial = std::any_of(std::begin(pack), std::end(pack),
									   [](const auto& dep_pack) { return dep_pack.allow_partial(); });

		if(has_partial && information.threading() == threading::par)
		{
			for(auto& dep_pack : pack)
			{
				auto entities = filter(dep_pack);
				if(entities.size() == 0) continue;

				cache_offset += prepare_bindings(entities, (void*)cache_offset, dep_pack);
			}

			auto multi_pack = slice(pack, m_Scheduler->workers());

			// std::vector<std::future<void>> future_commands;

			auto index = info_buffer.size();
			for(size_t i = 0; i < m_Scheduler->workers(); ++i)
				info_buffer.emplace_back(new info(*this, dTime, rTime, m_Tick));

			auto infoBuffer = std::next(std::begin(info_buffer), index);

			for(auto& mPack : multi_pack)
			{
				auto t1 = m_Scheduler->schedule([& fn = information.system(), infoBuffer, mPack]() mutable {
					return std::invoke(fn, infoBuffer->get(), mPack);
				});
				auto t2 = m_Scheduler->schedule(
					[&write_data, this, mPack = mPack]() { return std::invoke(write_data, *this, mPack); });

				t2.after(t1);

				// future_commands.emplace_back(std::move(t1));
				infoBuffer = std::next(infoBuffer);
			}
			m_Scheduler->execute();
		}
		else
		{
			for(auto& dep_pack : pack)
			{
				auto entities = filter(dep_pack);
				if(entities.size() == 0) continue;

				cache_offset += prepare_bindings(entities, (void*)cache_offset, dep_pack);
			}
			info_buffer.emplace_back(new info(*this, dTime, rTime, m_Tick));
			information.operator()(*info_buffer[info_buffer.size() - 1], pack);

			write_data(*this, pack);
		}
	//}
}
void state::tick(std::chrono::duration<float> dTime)
{
	for(auto& cInfo : m_Components) cInfo->lock();
	// tick systems;
	for(auto& system : m_SystemInformations)
	{
		prepare_system(dTime, dTime, (std::uintptr_t)m_Cache.data(), system);
	}

	if(m_LockOrphans > 0)
	{
		m_Entities[m_LockHead] = m_Next;
		m_Next				   = m_LockNext;
		m_Orphans += m_LockOrphans;
		m_LockOrphans = 0;
	}

	for(auto& info : info_buffer)
	{
		execute_command_buffer(*info);
	}
	info_buffer.clear();

	// purge;
	for(auto& cInfo : m_Components) cInfo->unlock_and_purge();

	++m_Tick;
}

const details::component_info* state::get_component_info(details::component_key_t key) const noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });


	return (it != std::end(m_Components)) ? it->operator->() : nullptr;
}

details::component_info* state::get_component_info(details::component_key_t key) noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });

	return (it != std::end(m_Components)) ? it->operator->() : nullptr;
}


psl::array<const details::component_info*>
state::get_component_info(psl::array_view<details::component_key_t> keys) const noexcept
{
	psl::array<const details::component_info*> res{};
	size_t count = keys.size();
	for(const auto& cInfo : m_Components)
	{
		if(auto it = std::find(std::begin(keys), std::end(keys), cInfo->id()); it != std::end(keys))
		{
			res.push_back(cInfo.operator->());
			--count;
		}
	}
	return (count == 0) ? res : psl::array<const details::component_info*>{};
}
// empty construction
void state::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
							   size_t size)
{
	auto cInfo = get_component_info(key);

	cInfo->add(entities);
}

// invocable based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
							   size_t size, std::function<void(std::uintptr_t, size_t)> invocable)
{
	assert(size != 0);
	auto cInfo = get_component_info(key);
	assert(cInfo != nullptr);

	auto offset = cInfo->entities(true).size();
	cInfo->add(entities);

	auto location = (std::uintptr_t)cInfo->data() + (offset * size);
	std::invoke(invocable, location, entities.size());
}

// prototype based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
							   size_t size, void* prototype)
{
	assert(size != 0);
	auto cInfo = get_component_info(key);
	assert(cInfo != nullptr);

	auto offset = cInfo->entities(true).size();
	cInfo->add(entities);
	for(const auto& id_range : entities)
	{
		for(auto i = id_range.first; i < id_range.second; ++i)
		{
			std::memcpy((void*)((std::uintptr_t)cInfo->data() + (offset++) * size), prototype, size);
		}
	}
}


// empty construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size)
{
	auto cInfo = get_component_info(key);
	assert(cInfo != nullptr);

	cInfo->add(entities);
}

// invocable based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
							   std::function<void(std::uintptr_t, size_t)> invocable)
{
	assert(size != 0);
	auto cInfo = get_component_info(key);
	assert(cInfo != nullptr);

	auto offset = cInfo->entities(true).size();
	cInfo->add(entities);

	auto location = (std::uintptr_t)cInfo->data() + (offset * size);
	std::invoke(invocable, location, entities.size());
}

// prototype based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
							   void* prototype)
{
	assert(size != 0);
	auto cInfo = get_component_info(key);
	assert(cInfo != nullptr);

	auto offset = cInfo->entities(true).size();

	cInfo->add(entities);
	for(auto e : entities)
	{
		std::memcpy((void*)((std::uintptr_t)cInfo->data() + (offset++) * size), prototype, size);
	}
}


void state::remove_component(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities) noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });
	assert(it != std::end(m_Components));
	(*it)->destroy(entities);
}


void state::remove_component(details::component_key_t key, psl::array_view<entity> entities) noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });
	assert(it != std::end(m_Components));


	(*it)->destroy(entities);
}


void state::destroy(psl::array_view<std::pair<entity, entity>> entities) noexcept
{
	auto count = std::accumulate(std::begin(entities), std::end(entities), entity{0},
								 [](entity sum, const auto& range) { return sum + (range.second - range.first); });
	for(auto& cInfo : m_Components)
	{
		cInfo->destroy(entities);
	}

	if(m_LockOrphans == 0) m_LockHead = entities[0].first;

	for(auto range : entities)
	{
		for(auto e = range.first; e < range.second; ++e)
		{
			m_Entities[e] = m_LockNext;
			m_LockNext	= e;
		}
		m_LockOrphans += range.second - range.first;
	}
}

// consider an alias feature
// ie: alias transform = position, rotation, scale components
void state::destroy(psl::array_view<entity> entities) noexcept
{
	if(entities.size() == 0) return;

	for(auto& cInfo : m_Components)
	{
		cInfo->destroy(entities);
	}
	if(m_LockOrphans == 0) m_LockHead = entities[0];
	m_LockOrphans += entities.size();
	for(auto e : entities)
	{
		m_Entities[e] = m_LockNext;
		m_LockNext	= e;
	}
}

void state::destroy(entity entity) noexcept
{
	for(auto& cInfo : m_Components)
	{
		cInfo->destroy(entity);
	}
	if(m_LockOrphans == 0) m_LockHead = entity;

	m_LockOrphans += 1;
	m_Entities[entity] = m_LockNext;
	m_LockNext		   = entity;
}


void state::fill_in(details::component_key_t key, psl::array_view<entity> entities,
					psl::array_view<std::uintptr_t>& data)
{}

psl::array<entity>::iterator state::filter_remove(details::component_key_t key, psl::array<entity>::iterator& begin,
												  psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? begin
							  : std::remove_if(begin, end, [cInfo](entity e) { return !cInfo->has_component(e); });
}

psl::array<entity>::iterator state::filter_remove_on_add(details::component_key_t key,
														 psl::array<entity>::iterator& begin,
														 psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? begin : std::remove_if(begin, end, [cInfo](entity e) { return !cInfo->has_added(e); });
}
psl::array<entity>::iterator state::filter_remove_on_remove(details::component_key_t key,
															psl::array<entity>::iterator& begin,
															psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? begin
							  : std::remove_if(begin, end, [cInfo](entity e) { return !cInfo->has_removed(e); });
}
psl::array<entity>::iterator state::filter_remove_except(details::component_key_t key,
														 psl::array<entity>::iterator& begin,
														 psl::array<entity>::iterator& end) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? begin
							  : std::remove_if(begin, end, [cInfo](entity e) { return cInfo->has_component(e); });
}
psl::array<entity>::iterator state::filter_remove_on_break(psl::array<details::component_key_t> keys,
														   psl::array<entity>::iterator& begin,
														   psl::array<entity>::iterator& end) const noexcept
{
	auto cInfos = get_component_info(keys);

	return (cInfos.size() == 0) ? begin : std::remove_if(begin, end, [cInfos](entity e) {
		return !std::any_of(std::begin(cInfos), std::end(cInfos), [e](const details::component_info* cInfo) {
			return cInfo->has_removed(e);
		}) || !std::all_of(std::begin(cInfos), std::end(cInfos), [e](const details::component_info* cInfo) {
			return cInfo->has_component(e) || cInfo->has_removed(e);
		});
	});
}

psl::array<entity>::iterator state::filter_remove_on_combine(psl::array<details::component_key_t> keys,
															 psl::array<entity>::iterator& begin,
															 psl::array<entity>::iterator& end) const noexcept
{
	auto cInfos = get_component_info(keys);

	return (cInfos.size() == 0) ? begin : std::remove_if(begin, end, [cInfos](entity e) {
		return !std::any_of(std::begin(cInfos), std::end(cInfos),
							[e](const details::component_info* cInfo) { return cInfo->has_added(e); }) ||
			   !std::all_of(std::begin(cInfos), std::end(cInfos),
							[e](const details::component_info* cInfo) { return cInfo->has_component(e); });
	});
}

psl::array<entity> state::filter_seed(details::component_key_t key) const noexcept
{
	assert(key != details::key_for<entity>());
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? psl::array<entity>{} : psl::array<entity>{cInfo->entities()};
}

psl::array<entity> state::filter_seed_on_add(details::component_key_t key) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? psl::array<entity>{} : psl::array<entity>{cInfo->added_entities()};
}

psl::array<entity> state::filter_seed_on_remove(details::component_key_t key) const noexcept
{
	auto cInfo = get_component_info(key);
	return (cInfo == nullptr) ? psl::array<entity>{} : psl::array<entity>{cInfo->removed_entities()};
}

psl::array<entity> state::filter_seed_on_break(psl::array<details::component_key_t> keys) const noexcept
{
	auto cInfos = get_component_info(keys);
	if(cInfos.size() == 0) return psl::array<entity>{};
	psl::array<entity> storage{cInfos[0]->entities(true)};
	auto begin = std::begin(storage);
	auto end   = std::end(storage);
	end		   = std::remove_if(begin, end, [cInfos](entity e) {
		   return std::none_of(std::begin(cInfos), std::end(cInfos),
							   [e](const details::component_info* cInfo) { return cInfo->has_removed(e); }) ||
				  std::any_of(std::begin(cInfos), std::end(cInfos), [e](const details::component_info* cInfo) {
					  return !(cInfo->has_component(e) || cInfo->has_removed(e));
				  });
	});
	storage.erase(end, std::end(storage));
	return storage;
}

psl::array<entity> state::filter_seed_on_combine(psl::array<details::component_key_t> keys) const noexcept
{
	auto cInfos = get_component_info(keys);
	if(cInfos.size() == 0) return psl::array<entity>{};
	psl::array<entity> storage{cInfos[0]->entities()};
	auto begin = std::begin(storage);
	auto end   = std::end(storage);
	end		   = std::remove_if(begin, end, [cInfos](entity e) {
		   return !std::any_of(std::begin(cInfos), std::end(cInfos),
							   [e](const details::component_info* cInfo) { return cInfo->has_added(e); }) ||
				  !std::all_of(std::begin(cInfos), std::end(cInfos),
							   [e](const details::component_info* cInfo) { return cInfo->has_component(e); });
	});
	storage.erase(end, std::end(storage));
	return storage;
}

bool state::filter_seed_best(psl::array_view<details::component_key_t> filters,
							 psl::array_view<details::component_key_t> added,
							 psl::array_view<details::component_key_t> removed, psl::array_view<entity>& out,
							 details::component_key_t& selected) const noexcept
{
	size_t count{std::numeric_limits<size_t>::max()};

	auto selection = [this, &count, &selected, &out, all_entities = removed.size() > 0](details::component_key_t key) {
		const auto& cInfo = get_component_info(key);
		if(cInfo == nullptr)
		{
			count = 0;
			return;
		}
		if(cInfo->entities().size() < count)
		{
			out		 = cInfo->entities(all_entities);
			count	= out.size();
			selected = cInfo->id();
		}
	};

	auto selection_add = [this, &count, &selected, &out](details::component_key_t key) {
		const auto& cInfo = get_component_info(key);
		if(cInfo == nullptr)
		{
			count = 0;
			return;
		}
		if(cInfo->added_entities().size() < count)
		{
			out		 = cInfo->added_entities();
			count	= out.size();
			selected = cInfo->id();
		}
	};

	auto selection_remove = [this, &count, &selected, &out](details::component_key_t key) {
		const auto& cInfo = get_component_info(key);
		if(cInfo == nullptr)
		{
			count = 0;
			return;
		}
		if(cInfo->removed_entities().size() < count)
		{
			out		 = cInfo->removed_entities();
			count	= out.size();
			selected = cInfo->id();
		}
	};


	std::for_each(std::begin(filters), std::end(filters), selection);
	std::for_each(std::begin(added), std::end(added), selection_add);
	std::for_each(std::begin(removed), std::end(removed), selection_remove);

	return count != std::numeric_limits<size_t>::max() && count != 0;
}

psl::array<std::pair<details::instruction, details::component_key_t>>::const_iterator
state::smallest_entity_list(const std::vector<std::pair<details::instruction, details::component_key_t>>& filters) const
	noexcept
{
	auto result  = std::end(filters);
	size_t count = std::numeric_limits<size_t>::max();
	for(auto it = std::begin(filters); it != std::end(filters); it = std::next(it))
	{
		const auto& cInfo = get_component_info(it->second);
		if(cInfo == nullptr)
		{
			return std::end(filters);
		}
		switch(it->first)
		{
		case details::instruction::EXCEPT:
		case details::instruction::FILTER:
			if(cInfo->entities().size() < count) result = it;
			break;
		case details::instruction::ADD:
		case details::instruction::COMBINE:
			if(cInfo->added_entities().size() < count) result = it;
			break;
		case details::instruction::REMOVE:
			if(cInfo->removed_entities().size() < count) result = it;
			break;
		case details::instruction::BREAK:
			if(cInfo->entities(true).size() < count) result = it;
			break;
		}
	}

	return result;
}
psl::array<entity> state::initial_filter(const details::dependency_pack& pack) const noexcept
{
	psl::array<std::pair<details::instruction, details::component_key_t>> instructions;
	for(const auto& filter : pack.filters) instructions.emplace_back(details::instruction::FILTER, filter);
	for(const auto& filter : pack.on_add) instructions.emplace_back(details::instruction::ADD, filter);
	for(const auto& filter : pack.on_remove) instructions.emplace_back(details::instruction::REMOVE, filter);
	for(const auto& filter : pack.on_combine) instructions.emplace_back(details::instruction::COMBINE, filter);
	for(const auto& filter : pack.on_break) instructions.emplace_back(details::instruction::BREAK, filter);
	// for(const auto& filter : pack.except) instructions.emplace_back(details::instruction::EXCEPT, filter);

	if(auto it = smallest_entity_list(instructions); it != std::end(instructions))
	{
		const auto& cInfo = get_component_info(it->second);
		switch(it->first)
		{
		// case details::instruction::EXCEPT:
		case details::instruction::FILTER: return psl::array<entity>{cInfo->entities()}; break;
		case details::instruction::ADD:
		case details::instruction::COMBINE: return psl::array<entity>{cInfo->added_entities()}; break;
		case details::instruction::REMOVE: return psl::array<entity>{cInfo->removed_entities()}; break;
		case details::instruction::BREAK: return psl::array<entity>{cInfo->entities(true)}; break;
		}
	}
	return {};
}

bool state::filter_seed_best(const details::dependency_pack& pack, psl::array_view<entity>& out,
							 details::component_key_t& selected, details::instruction& instruction) const noexcept
{
	psl::array<std::pair<details::instruction, details::component_key_t>> instructions;
	for(const auto& filter : pack.filters) instructions.emplace_back(details::instruction::FILTER, filter);
	for(const auto& filter : pack.on_add) instructions.emplace_back(details::instruction::ADD, filter);
	for(const auto& filter : pack.on_remove) instructions.emplace_back(details::instruction::REMOVE, filter);
	for(const auto& filter : pack.on_combine) instructions.emplace_back(details::instruction::COMBINE, filter);
	for(const auto& filter : pack.on_break) instructions.emplace_back(details::instruction::BREAK, filter);
	for(const auto& filter : pack.except) instructions.emplace_back(details::instruction::EXCEPT, filter);

	auto it = smallest_entity_list(instructions);
	if(it == std::end(instructions)) return false;
	instruction		  = it->first;
	const auto& cInfo = get_component_info(it->second);
	switch(it->first)
	{
	case details::instruction::EXCEPT:
	case details::instruction::FILTER: out = cInfo->entities(); break;
	case details::instruction::ADD:
	case details::instruction::COMBINE: out = cInfo->added_entities(); break;
	case details::instruction::REMOVE: out = cInfo->removed_entities(); break;
	case details::instruction::BREAK: out = cInfo->entities(true); break;
	}
	selected = it->second;

	return true;
}

psl::array<std::pair<details::instruction, psl::array<details::component_key_t>>>
state::to_instructions(const details::dependency_pack& pack) const noexcept
{
	using details::instruction;
	psl::array<std::pair<details::instruction, psl::array<details::component_key_t>>> res;
	for(const auto& filter : pack.filters)
		res.emplace_back(instruction::FILTER, psl::array<details::component_key_t>{filter});

	for(const auto& filter : pack.on_add)
		res.emplace_back(instruction::ADD, psl::array<details::component_key_t>{filter});

	for(const auto& filter : pack.on_remove)
		res.emplace_back(instruction::REMOVE, psl::array<details::component_key_t>{filter});


	for(const auto& filter : pack.except)
		res.emplace_back(instruction::EXCEPT, psl::array<details::component_key_t>{filter});
	res.emplace_back(instruction::COMBINE, pack.on_combine);
	res.emplace_back(instruction::BREAK, pack.on_break);

	return res;
}

psl::array<entity> state::filter_seed(
	psl::array<std::pair<details::instruction, psl::array<details::component_key_t>>>& instructions) const noexcept
{
	/*std::sort(std::begin(instructions), std::end(instructions),
			  [this](const std::pair<details::instruction, psl::array<details::component_key_t>>& instruction) {
				  switch(instruction.first)
				  {
				  case details::instruction::FILTER:
				  {
					  const auto& cInfo = get_component_info(instruction.second[0]);
					  return cInfo->entities().size();
				  }
				  break;
				  case details::instruction::ADD:
				  {
					  const auto& cInfo = get_component_info(instruction.second[0]);
					  return cInfo->added_entities().size();
				  }
				  break;
				  case details::instruction::REMOVE:
				  {
					  const auto& cInfo = get_component_info(instruction.second[0]);
					  return cInfo->removed_entities().size();
				  }
				  break;
				  case details::instruction::EXCEPT:
				  case details::instruction::BREAK:
				  case details::instruction::COMBINE: return std::numeric_limits<size_t>::max(); break;
				  }
			  });*/

	return {};
}


psl::array<entity>::iterator state::filter(psl::array<entity>::iterator begin, psl::array<entity>::iterator end,
										   const details::dependency_pack& pack) const noexcept
{
	std::unordered_set<details::component_key_t> processed{};
	for(auto filter : pack.on_remove)
	{
		if(processed.find(filter) != std::end(processed)) continue;
		processed.emplace(filter);
		end = filter_remove_on_remove(filter, begin, end);
	}
	if(pack.on_break.size() > 0)
	{
		end = filter_remove_on_break(pack.on_break, begin, end);
	}
	for(auto filter : pack.on_add)
	{
		if(processed.find(filter) != std::end(processed)) continue;
		processed.emplace(filter);
		end = filter_remove_on_add(filter, begin, end);
	}
	if(pack.on_combine.size() > 0)
	{
		end = filter_remove_on_combine(pack.on_combine, begin, end);
	}
	for(auto filter : pack.filters)
	{
		if(processed.find(filter) != std::end(processed)) continue;
		processed.emplace(filter);
		end = filter_remove(filter, begin, end);
	}

	for(auto filter : pack.except)
	{
		if(processed.find(filter) != std::end(processed)) continue;
		processed.emplace(filter);
		end = filter_remove_except(filter, begin, end);
	}
	for(const auto& conditional : pack.on_condition)
	{
		end = std::invoke(conditional, begin, end, *this);
	}

	// std::invoke(pack.orderby, begin, end, *this);

	return end;
}

psl::array<entity> state::filter(details::dependency_pack& pack) const noexcept
{
	// auto instructions{to_instructions(pack)};

	details::instruction instruction;
	psl::array_view<entity> best_pack{};
	details::component_key_t best_key;
	if(!filter_seed_best(pack, best_pack, best_key, instruction)) return psl::array<entity>{};
	psl::array<entity> result{best_pack};
	auto begin = std::begin(result);
	auto end   = std::end(result);
	std::unordered_set<details::component_key_t> processed{best_key};
	for(auto filter : pack.on_remove)
	{
		if(processed.find(filter) != std::end(processed)) continue;
		processed.emplace(filter);
		end = filter_remove_on_remove(filter, begin, end);
	}
	if(pack.on_break.size() > 0)
	{
		for(auto filter : pack.on_break) processed.insert(filter);
		end = filter_remove_on_break(pack.on_break, begin, end);
	}
	for(auto filter : pack.on_add)
	{
		if(processed.find(filter) != std::end(processed)) continue;
		processed.emplace(filter);
		end = filter_remove_on_add(filter, begin, end);
	}
	if(pack.on_combine.size() > 0) end = filter_remove_on_combine(pack.on_combine, begin, end);
	for(auto filter : pack.filters)
	{
		if(processed.find(filter) != std::end(processed)) continue;
		processed.emplace(filter);
		end = filter_remove(filter, begin, end);
	}
	for(auto filter : pack.except)
	{
		end = filter_remove_except(filter, begin, end);
	}

	for(const auto& conditional : pack.on_condition)
	{
		end = std::invoke(conditional, begin, end, *this);
	}

	std::invoke(pack.orderby, begin, end, *this);

	result.erase(end, std::end(result));
	return result;
}

size_t state::prepare_data(psl::array_view<entity> entities, void* cache, component_key_t id) const noexcept
{
	if(entities.size() == 0) return 0;
	const auto& cInfo = get_component_info(id);
	assert_debug_break(cInfo != nullptr);
	return cInfo->copy_to(entities, cache);
}

size_t state::prepare_bindings(psl::array_view<entity> entities, void* cache, details::dependency_pack& dep_pack) const
	noexcept
{
	size_t offset_start = (std::uintptr_t)cache;

	std::memcpy(cache, entities.data(), sizeof(entity) * entities.size());
	dep_pack.m_Entities =
		psl::array_view<entity>((entity*)cache, (entity*)((std::uintptr_t)cache + (sizeof(entity) * entities.size())));

	cache = (void*)((std::uintptr_t)cache + (sizeof(entity) * entities.size()));


	for(auto& binding : dep_pack.m_RBindings)
	{
		std::uintptr_t data_begin = (std::uintptr_t)cache;
		auto write_size			  = prepare_data(entities, cache, binding.first);
		cache					  = (void*)((std::uintptr_t)cache + write_size);
		binding.second = psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache);
	}
	for(auto& binding : dep_pack.m_RWBindings)
	{
		std::uintptr_t data_begin = (std::uintptr_t)cache;
		auto write_size			  = prepare_data(entities, cache, binding.first);
		cache					  = (void*)((std::uintptr_t)cache + write_size);
		binding.second = psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache);
	}
	return (std::uintptr_t)cache - offset_start;
}

size_t state::set(psl::array_view<entity> entities, details::component_key_t key, void* data) noexcept
{
	if(entities.size() == 0) return 0;
	const auto& cInfo = get_component_info(key);
	assert(cInfo != nullptr);
	return cInfo->copy_from(entities, data);
}


void state::execute_command_buffer(info& info)
{
	auto& buffer = info.command_buffer;

	psl::sparse_array<entity> remapped_entities;
	if(buffer.m_Entities.size() > 0)
	{
		psl::array<entity> added_entities;
		std::set_difference(std::begin(buffer.m_Entities), std::end(buffer.m_Entities),
							std::begin(buffer.m_DestroyedEntities),
							std::end(buffer.m_DestroyedEntities), std::back_inserter(added_entities));


		for(auto e : added_entities)
		{
			remapped_entities[e] = create();
		}
	}
	for(auto& component_src : buffer.m_Components)
	{
		if(!component_src->changes()) continue;
		auto component_dst = get_component_info(component_src->id());
		if(component_dst == nullptr)
		{
			m_Components.emplace_back(component_src->create_storage(remapped_entities));
		}
		else
		{
			component_src->merge_into(component_dst, remapped_entities);
		}
	}
	for(auto e : buffer.m_DestroyedEntities)
	{
		if(e < buffer.m_First) destroy(e);
	}
}


size_t state::count(psl::array_view<details::component_key_t> keys) const noexcept
{
	for(auto& key : keys)
	{

		auto cInfo = get_component_info(key);
		return cInfo->size();
	}
	return 0;
}