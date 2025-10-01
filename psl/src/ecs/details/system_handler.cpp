#include "psl/ecs/details/system_handler.hpp"
#include "psl/ecs/state.hpp"
#include <new>
#include <tbb/flow_graph.h>
#include <tbb/rw_mutex.h>

#include "tracy/Tracy.hpp"

#include "fmt/format.h"

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size  = 64;
#endif


#define ZoneNameFmt(formatting, ...)                                                                                   \
	auto _val {::fmt::format(formatting, ##__VA_ARGS__)};                                                              \
	ZoneName(_val.data(), _val.size());

//	Initial Graph:
//	filter_node_t 1 ─┬─→ system_group_t ─→ (creates nodes dynamically)
//	filter_node_t 2 ─┤
//	                 └─→ system_group_t ─→ (creates nodes dynamically)
//
//	After SplitA decides to create 3 chunks:
//	filter_node_t 1 ─┬─→ system_group_t ─┬─→ CacheA_0 ─→ ExecuteA_0
//	filter_node_t 2 ─┤                   ├─→ CacheA_1 ─→ ExecuteA_1
//	                 │                   └─→ CacheA_2 ─→ ExecuteA_2
//	                 └─→ system_group_t ─→ (still pending)
//
//	After SplitB decides to create 2 chunks:
//	filter_node_t 1 ─┬─→ system_group_t ─┬─→ CacheA_0 ─→ ExecuteA_0
//	filter_node_t 2 ─┤                   ├─→ CacheA_1 ─→ ExecuteA_1
//	                 │                   └─→ CacheA_2 ─→ ExecuteA_2
//	                 └─→ system_group_t ─┬─→ CacheB_0 ─→ ExecuteB_0 (main thread)
//	                                     └─→ CacheB_1 ─→ ExecuteB_1 (main thread)

namespace psl::ecs::details {

system_handler_t::system_handler_t(options settings)
	: m_SystemScheduler(m_FilteringTasks, settings.min_entities_per_worker),
	  m_Arena(settings.workers == 0 ? tbb::task_arena::automatic : settings.workers),
	  m_Cache(settings.cache_size, hardware_destructive_interference_size) {}

component_lock_instance_group_t make_locks(tbb::mutex& master_lock,
										   details::system_information& system,
										   std::unordered_map<component_key_t, component_lock_t>& cache) {
	auto pack = system.create_pack();

	std::vector<component_lock_instance_t> locks {};
	for(auto const& entry : pack) {
		auto bindings = entry.get_bindings();
		for(auto const& binding : bindings) {
			auto lock_it = cache.find(binding.id);
			if(lock_it == cache.end()) {
				auto [it, success] = cache.emplace(binding.id, component_lock_t {});
				lock_it			   = it;
				psl_assert(success, "Failed to insert a new component lock");
			}
			locks.emplace_back(&lock_it->second, system.id(), !binding.is_read_only, !binding.is_indirect);
		}
	}
	return component_lock_instance_group_t(locks, master_lock);
}
void system_handler_t::internal_graph_t::prepare(filter_shared_state_t const& state,
												 cache_resource_t& cache,
												 components_cache_t* components_cache) {
	m_FilterState	 = state;
	m_Cache			 = &cache;
	m_ComponentCache = components_cache;
}
void system_handler_t::internal_graph_t::add(system_information* system, psl::array<filter_result>& all_filters) {
	if(has(system->id())) {
		return;
	}
	m_ExistingSystems.insert(system->id());
	auto get_filter_id = [&all_filters](std::shared_ptr<psl::ecs::details::filter_group> const& filter) -> filter_id_t {
		auto filter_it = std::find_if(begin(all_filters), end(all_filters), [&filter](const auto& data) {
			return data.group && *data.group == *filter;
		});
		return filter_it->id;
	};

	auto get_all_filter_ids =
	  [&get_filter_id](
		psl::array<std::shared_ptr<psl::ecs::details::filter_group>> const& filters) -> std::vector<filter_id_t> {
		std::vector<filter_id_t> result {};
		result.reserve(filters.size());
		for(auto const& filter : filters) {
			result.push_back(get_filter_id(filter));
		}
		return result;
	};

	auto filters = get_all_filter_ids(system->filters());
	auto& task =
	  m_SystemContainers.emplace_back(std::make_unique<system_task_container_t>(system->id(), system, filters.size()));
	auto res = m_SystemMap.emplace(
	  system->id(),
	  system_info_t {system,
					 filters,
					 {},
					 std::vector<system_token>(system->id().dependencies().begin(), system->id().dependencies().end()),
					 m_SystemContainers.back().get()});

	std::vector<filter_id_t> missing_filters {};
	for(auto filter_id : filters) {
		if(m_FilterMap.find(filter_id) == m_FilterMap.end()) {
			missing_filters.push_back(filter_id);
		}
		m_FilterMap[filter_id].systems.emplace_back(system->id(), &res.first->second);
	}
	if(filters.empty()) {
		m_FilterlessSystems.insert(system->id());
	}

	for(auto id : missing_filters) {
		auto& filter = m_FilterMap[id];
		auto filter_it =
		  std::find_if(all_filters.begin(), all_filters.end(), [id](const auto& filter) { return filter.id == id; });
		psl_assert(filter_it != all_filters.end(), "Filter id {} was not found in the provided filters", id);
		m_FilterResults.emplace_back(std::make_unique<filter_task_result_t>(&*filter_it, 1, id));
		filter.result = m_FilterResults.back().get();
	}

	for(auto const& filter : filters) {
		res.first->second.filters_task_results.push_back(m_FilterMap[filter].result);
		res.first->second.task->filters.push_back(m_FilterMap[filter].result);
	}

	m_RunningSystems.emplace(
	  system->id(),
	  std::make_unique<system_invocable_task_group_t>(
		system->id(), system, make_locks(m_ComponentLockMutex, *system, m_ComponentLocks), m_Cache, m_ComponentCache));
}

void system_handler_t::internal_graph_t::remove(filter_id_t filter) {
	if(auto it = m_FilterMap.find(filter); it != m_FilterMap.end()) {
		m_FilterResults.erase(std::remove_if(m_FilterResults.begin(),
											 m_FilterResults.end(),
											 [filter](auto const& result) { return result->id == filter; }),
							  m_FilterResults.end());
		m_FilterMap.erase(it);
	}
}

void system_handler_t::internal_graph_t::remove(system_token system) {
	if(!has(system)) {
		return;
	}
	m_ExistingSystems.erase(system);
	if(auto it = m_SystemMap.find(system); it != m_SystemMap.end()) {
		auto& info = it->second;
		for(auto filter_id : info.filters) {
			auto filter_it = m_FilterMap.find(filter_id);
			if(filter_it != m_FilterMap.end()) {
				auto& systems = filter_it->second.systems;
				systems.erase(std::remove_if(systems.begin(),
											 systems.end(),
											 [system](auto const& pair) { return pair.first == system; }),
							  systems.end());
				if(systems.empty()) {
					remove(filter_id);
				}
			}
		}
		m_SystemContainers.erase(std::find_if(m_SystemContainers.begin(),
											  m_SystemContainers.end(),
											  [task = info.task](auto const& ptr) { return task == ptr.get(); }));
		m_SystemMap.erase(it);
	}
	m_RunningSystems.erase(system);
	m_FilterlessSystems.erase(system);
}

void system_handler_t::internal_graph_t::schedule(tbb::task_group& group, system_scheduler_t& system_scheduler) {
	ZoneScoped;
	system_scheduler.m_RemainingTasks = m_SystemMap.size();
	for(auto& [filter_id, filter_info] : m_FilterMap) {
		group.run([handler			 = m_FilterState.handler,
				   result			 = filter_info.result,
				   changeset		 = filter_info.result->container->group->is_hierarchy_change_active()
										 ? m_FilterState.mutated
										 : m_FilterState.modified,
				   systems			 = filter_info.systems,
				   &system_scheduler = system_scheduler]() {
			handler->filter(*const_cast<filter_result*>(result->container), changeset);
			// determine the amount of max amount workers to use for this filter
			// this doesn't mean the system will use that many, as other filters the system depends on might
			// influence this, as well as the historical execution time of the system.
			// And lastly the user also influences this when they mark the filter as "psl::ecs::full_t",
			// which cannot be split in those cases.
			{
				static constexpr auto min_entities_per_worker = 1 << 11;
				// todo(jdl): this should be load balanced based on past performance of the system
				auto const workers	   = size_t(std::thread::hardware_concurrency() * 2);
				auto const max_workers = std::max(
				  size_t {1},
				  std::min(workers,
						   (result->container->entities.size() -
							(result->container->entities.size() % system_scheduler.min_entities_per_worker())) /
							 system_scheduler.min_entities_per_worker()));
				result->workers = max_workers;
			}
			bool has_scheduled = false;
			for(auto& [id, info] : systems) {
				if(--info->task->pending_filters == 0) {
					system_scheduler.execute(id, info->system, info->filters_task_results);
					has_scheduled = true;
				}
			}
			if(has_scheduled) {
				system_scheduler.try_execute_pending();
			}
		});
	}

	for(auto const& system_id : m_FilterlessSystems) {
		auto it = m_SystemMap.find(system_id);
		psl_assert(it != m_SystemMap.end(), "system was not found in the system map");
		auto& info = it->second;
		psl_assert(info.task->pending_filters == 0, "system with filters was found in the filterless set");
		group.run(
		  [&system_scheduler = system_scheduler,
		   system_id,
		   system		= info.system,
		   task_results = info.filters_task_results]() { system_scheduler.execute(system_id, system, task_results); });
	}
}

void system_handler_t::internal_graph_t::apply_changes(psl::array<system_information*>& systems,
													   psl::array<filter_result>& all_filters) {
	ZoneScoped;
	std::unordered_set<system_token> to_remove = m_ExistingSystems;
	std::unordered_set<system_token> added_systems {};
	bool has_changes = false;
	for(auto& system : systems) {
		to_remove.erase(system->id());
		if(!has(system->id())) {
			add(system, all_filters);
			has_changes = true;
			added_systems.insert(system->id());
		}
	}
	for(auto const& system : to_remove) {
		remove(system);
	}
	std::unordered_set<filter_id_t> updated_filters {};
	for(auto& filter : m_FilterResults) {
		auto it = std::find_if(
		  all_filters.begin(), all_filters.end(), [id = filter->id](const auto& data) { return data.id == id; });
		if(it == all_filters.end()) {
			throw std::runtime_error(
			  fmt::format("Filter with id {} was not found in the provided filters", filter->id));
		}
		if(filter->container == &*it) {
			continue;
		}
		filter->container = &*it;
		updated_filters.insert(filter->id);
	}
	for(auto& [id, info] : m_SystemMap) {
		info.task->reset();
	}
	for(auto& [id, sys] : m_RunningSystems) {
		sys->reset();
	}

	// we do this at the end so we know the systems we depend on are already present.
	for(auto const& id : added_systems) {
		std::vector<system_invocable_task_group_t*> dependencies {};
		dependencies.reserve(id.dependencies().size());
		for(auto const& dependency : id.dependencies()) {
			auto it = m_RunningSystems.find(dependency);
			if(it != m_RunningSystems.end()) {
				dependencies.push_back(it->second.get());
			}
		}
		auto running_it = m_RunningSystems.find(id);
		running_it->second->set_dependencies(std::move(dependencies));
	}
}


void system_handler_t::internal_graph_t::remap_filters(psl::array<std::pair<filter_id_t, filter_id_t>> const& remaps) {
	for(auto [original, dest] : remaps) {
		auto& info = m_FilterMap[original];
		for(auto& [system_id, system_info] : info.systems) {
			auto& sys	 = m_SystemMap[system_id];
			auto it		 = std::find(sys.filters.begin(), sys.filters.end(), original);
			*it			 = dest;
			auto task_it = std::find_if(sys.task->filters.begin(),
										sys.task->filters.end(),
										[original](auto const& filter_res) { return filter_res->id == original; });
			*task_it	 = m_FilterMap[dest].result;
		}


		m_FilterResults.erase(std::find_if(m_FilterResults.begin(),
										   m_FilterResults.end(),
										   [original](auto const& result) { return result->id == original; }));

		m_FilterMap.erase(original);
	}
}

void system_handler_t::remap_filters(psl::array<std::pair<filter_id_t, filter_id_t>> const& remaps) {
	m_Graph.remap_filters(remaps);
}

auto system_handler_t::execute(state_info_t info,
							   psl::array<system_information*> systems,
							   std::chrono::duration<float> dTime,
							   std::chrono::duration<float> rTime,
							   size_t tick) -> psl::array<std::unique_ptr<psl::ecs::info_t>> {
	ZoneScoped;
	if(!info.state) {
		throw std::runtime_error("You need to provide a valid state to execute systems");
	}
	m_Graph.prepare({info.filter_handler, info.modified_entities, info.mutated_entities}, m_Cache, info.state);
	m_Graph.apply_changes(systems, info.filter_handler->get_filter_results());
	m_SystemScheduler.prepare(
	  *info.state,
	  dTime,
	  rTime,
	  tick,
	  &m_Graph.running_systems());	  // consider the calling thread the main thread for this run

	m_Arena.execute([this, &graph = m_Graph, &task_group = m_FilteringTasks, &scheduler = m_SystemScheduler]() {
		graph.schedule(task_group, scheduler);

		// In case there are not enough tasks available to keep all the threads busy, we might
		// end up with the main thread never participating. This last call will ensure that
		// the main thread will help out if needed.
		task_group.wait();
		size_t const max_tries = 1000;
		size_t tries		   = 0;
		while(!m_SystemScheduler.is_done()) {
			task_group.run_and_wait([this, &scheduler = scheduler]() { scheduler.try_execute_pending(); });
			if(++tries >= max_tries) {
				throw std::runtime_error("System execution deadlock detected");
			}
		}
	});

	psl_assert(m_SystemScheduler.is_done(), "System scheduler is not done after execution");
	psl::array<std::unique_ptr<psl::ecs::info_t>> results {};
	results.reserve(m_SystemScheduler.m_CommandBuffers.size());
	for(auto& buffer : m_SystemScheduler.m_CommandBuffers) {
		results.push_back(std::move(buffer));
	}
	m_SystemScheduler.m_CommandBuffers.clear();
	return results;
}


std::vector<std::pair<system_token, system_invocable_task_t::metrics_t>>
system_handler_t::get_metrics() const noexcept {
	std::vector<std::pair<system_token, system_invocable_task_t::metrics_t>> result {};
	result.reserve(m_Graph.running_systems().size());
	for(auto const& [id, system] : m_Graph.running_systems()) {
		result.emplace_back(id, system->get_metrics());
	}
	return result;
}

std::vector<system_invocable_task_t> system_scheduler_t::make_tasks(system_token id,
																	system_information* system,
																	std::vector<filter_task_result_t*> filters) {
	std::vector<psl::array_view<psl::ecs::entity_t>> entities;
	for(auto i = 0u; i < filters.size(); ++i) {
		auto filter_result	 = filters[i]->container;
		auto transform_group = std::next(system->transforms().begin(), i)->get();


		if(transform_group) {
			auto transform =
			  std::find_if(std::begin(filter_result->transformations),
						   std::end(filter_result->transformations),
						   [transform_group](const auto& data) { return *data.group == *transform_group; });
			entities.emplace_back(transform->entities);
		} else {
			entities.emplace_back(filter_result->entities);
		}
	}

	auto const suggested_workers =
	  filters.empty() ? 1
					  : (**std::min_element(filters.begin(),
											filters.end(),
											[](filter_task_result_t const* lhs, filter_task_result_t const* rhs) {
												return lhs->workers < rhs->workers;
											}))
						  .workers;

	auto prototype_pack = system->create_pack();
	std::vector<std::vector<dependency_pack>> packs {};
	packs.resize(prototype_pack.size());
	for(auto i = 0u; i < prototype_pack.size(); ++i) {
		if(prototype_pack[i].is_partial_pack() && suggested_workers > 1) {
			packs[i].reserve(suggested_workers);
			auto batch_size = entities[i].size() / suggested_workers;
			size_t processed {0};
			for(size_t u = 0; u < suggested_workers - 1; ++u) {
				packs[i].emplace_back(
				  prototype_pack[i].from_entities(entities[i].slice(processed, processed + batch_size)));
				processed += batch_size;
			}
			packs[i].emplace_back(
			  prototype_pack[i].from_entities(entities[i].slice(processed, filters[i]->container->entities.size())));
		} else {
			packs[i].emplace_back(prototype_pack[i].from_entities(entities[i]));
		}
	}

	// determine the amount of tasks to create for this system
	size_t const max_tasks =
	  packs.empty() ? 1 : std::max_element(std::begin(packs), std::end(packs), [](const auto& lhs, const auto& rhs) {
							  return lhs.size() < rhs.size();
						  })->size();

	std::vector<system_invocable_task_t> tasks {};
	tasks.reserve(max_tasks);
	for(auto i = 0u; i < max_tasks; ++i) {
		std::vector<dependency_pack> task_packs {};
		for(auto& pack : packs) {
			if(pack.size() == 1) {
				task_packs.push_back(pack[0]);
			} else {
				task_packs.push_back(pack[i]);
			}
		}
		tasks.emplace_back(system->is_const()
							 ? m_SharedCommandBuffer.get()
							 : m_CommandBuffers
								 .emplace_back(std::make_unique<psl::ecs::info_t>(m_SharedCommandBuffer->state,
																				  m_SharedCommandBuffer->dTime,
																				  m_SharedCommandBuffer->rTime,
																				  m_SharedCommandBuffer->tick,
																				  system->tick()))
								 ->get(),
						   task_packs);
	}
	return tasks;
}


void system_scheduler_t::schedule(psl::ecs::details::system_information* system, system_invocable_task_group_t& group) {
	if(system->threading() == psl::ecs::threading::main) {
		m_MainThreadExecutor.schedule([this, group = &group]() mutable -> bool {
			auto token = group->id();
			if(!std::all_of(
				 token.dependencies().begin(), token.dependencies().end(), [this](system_token const& token) {
					 return m_RunningSystems->find(token)->second->is_finished();
				 })) {
				return false;
			}
			auto res = group->operator()(this);
			if(res) {
				--m_RemainingTasks;
			}
			return res;
		});
	} else {
		m_PendingTaskGroups.push(&group);
	}
}

void system_scheduler_t::execute(system_token id,
								 system_information* system,
								 std::vector<filter_task_result_t*> filters) {
	auto system_it = m_RunningSystems->find(id);
	system_it->second->set_tasks(make_tasks(id, system, filters));
	if(system_it->second->is_ready()) {
		schedule(system, *system_it->second);
	}
}

constexpr auto align_offset(std::uintptr_t size, std::uintptr_t alignment) -> std::uintptr_t {
	if(alignment == 0) {
		return 0;
	}
	auto remainder = size % alignment;
	if(remainder == 0) {
		return 0;
	}
	return alignment - remainder;
};

std::vector<size_t> system_invocable_task_t::get_cache_requirements() const {
	std::vector<size_t> requirements {};
	requirements.reserve(m_Packs.size());
	for(auto const& pack : m_Packs) {
		requirements.push_back(pack.bindings_total_size());
	}
	return requirements;
}

void system_invocable_task_t::prepare(psl::string_view system_name,
									  std::vector<std::optional<memory::segment>> const& segments,
									  components_cache_t* components_cache) const {
	ZoneScoped;
	ZoneNameFmt("ecs::prepare::{}", system_name);
	// ZoneNameF("ecs::prepare::{}", (int)system_name.size(), system_name.data());
	psl_assert(segments.size() == m_Packs.size(),
			   "The number of segments provided does not match the number of packs in the task");
	auto write_fn = [components_cache, &segments](details::dependency_pack const& pack,
												  memory::segment const& segment) {
		std::uintptr_t data_begin = segment.range().begin;
		auto write_fn = [components_cache, entities = pack.m_Entities, &segment, &data_begin](auto& binding) {
			const auto& cInfo = components_cache->get_component_container(binding.first);
			if(cInfo->component_type_info().size > 0) {
				data_begin += align_offset(data_begin, cInfo->component_type_info().alignment);
				psl_assert(data_begin + cInfo->component_type_info().size * entities.size() <= segment.range().end,
						   "Not enough space allocated for the direct view");
				auto write_size = cInfo->copy_to(entities, (void*)(data_begin));
				auto data_end	= data_begin + write_size;
				binding.second =
				  psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)data_end);
				data_begin = data_end;
			}
		};

		auto view_fn = [components_cache, entities = pack.m_Entities, &segment, &data_begin](auto& binding) {
			const auto& cInfo = components_cache->get_component_container(binding.first);
			if(cInfo->component_type_info().size > 0) {
				data_begin += align_offset(data_begin, alignof(entity_t));
				psl_assert(data_begin + sizeof(entity_t) * entities.size() <= segment.range().end,
						   "Not enough space allocated for the indirect view");
				auto data_end = cInfo->write_memory_location_offsets_for(entities, (entity_t::size_type*)(data_begin));
				binding.second.indices = psl::array_view<entity_t::size_type>(
				  (entity_t::size_type*)data_begin, (entity_t::size_type*)((std::uintptr_t)data_end));
				binding.second.data = cInfo->data();

				data_begin = (std::uintptr_t)data_end;
			}
		};

		if(pack.is_direct_access()) {
			std::for_each(std::begin(pack.m_RBindings), std::end(pack.m_RBindings), write_fn);
			std::for_each(std::begin(pack.m_RWBindings), std::end(pack.m_RWBindings), write_fn);
		} else {
			std::for_each(std::begin(pack.m_IndirectReadBindings), std::end(pack.m_IndirectReadBindings), view_fn);
			std::for_each(
			  std::begin(pack.m_IndirectReadWriteBindings), std::end(pack.m_IndirectReadWriteBindings), view_fn);
		}
	};

	auto segment_it = segments.begin();
	auto pack_it	= m_Packs.begin();
	for(; segment_it != segments.end() && pack_it != m_Packs.end(); ++segment_it, ++pack_it) {
		if(!segment_it->has_value()) {
			continue;
		}
		write_fn(*pack_it, segment_it->value());
	}
}
void system_invocable_task_t::execute(psl::string_view system_name,
									  system_information* system,
									  psl::ecs::info_t* info) const {
	ZoneScoped;
	ZoneNameFmt("ecs::execute::{}", system_name);
	std::invoke(*system, *info, m_Packs);
}
void system_invocable_task_t::finalize(psl::string_view system_name, components_cache_t* components_cache) const {
	ZoneScoped;
	ZoneNameFmt("ecs::finalize::{}", system_name);
	for(const auto& dep_pack : m_Packs) {
		auto bindings = dep_pack.get_bindings();
		for(auto& binding : dep_pack.m_RWBindings) {
			std::uintptr_t data = (std::uintptr_t)binding.second.data();
			components_cache->component_copy_from(dep_pack.m_Entities, binding.first, (void*)data);
		}
	}
}

void system_scheduler_t::prepare(
  psl::ecs::state_t& state,
  std::chrono::duration<float> dTime,
  std::chrono::duration<float> rTime,
  size_t tick,
  std::unordered_map<system_token, std::unique_ptr<system_invocable_task_group_t>>* running_systems) noexcept {
	m_MainThreadExecutor.rebind();
	m_RunningSystems = running_systems;
	if(!m_SharedCommandBuffer || &state != &m_SharedCommandBuffer->state) {
		m_SharedCommandBuffer = std::make_unique<psl::ecs::info_t>(state, dTime, rTime, tick, 0);
	} else {
		m_SharedCommandBuffer->dTime = dTime;
		m_SharedCommandBuffer->rTime = rTime;
		m_SharedCommandBuffer->tick	 = tick;
	}
}
void system_scheduler_t::try_execute_pending() {
start:
	if(m_MainThreadExecutor.is_valid_thread()) {
		while(m_MainThreadExecutor.has_work()) {
			m_MainThreadExecutor.process();
		}
	}
	if(m_PendingTaskGroups.empty()) {
		return;
	}

	system_invocable_task_group_t* task_group {};
	while(m_PendingTaskGroups.try_pop(task_group)) {
		if(!task_group->operator()(this, m_ExecutionGroup)) {
			m_PendingTaskGroups.push(task_group);
		} else {
			--m_RemainingTasks;
		}
	}
	if(m_MainThreadExecutor.is_valid_thread() && m_RemainingTasks > 0) {
		goto start;
	}
}

bool system_invocable_task_group_t::operator()(system_scheduler_t* handler,
											   std::optional<std::reference_wrapper<tbb::task_group>> group) {
	std::unique_lock full_scope_lock(m_Mutex, std::try_to_lock);
	if(!full_scope_lock.owns_lock()) {
		return false;
	}
	if(m_Tasks.end() == m_Current) {
		return true;
	}
	psl_assert(is_ready(), "System task group was not ready to be executed");
	// already running tasks, we can safely ignore taking a lock
	// the idea is we have the m_Locks enabled when leaving this scope
	// as long as there's pending tasks. The last pending task will
	// release the locks.
	if(m_PendingTasks == 0 && !m_Locks.try_lock()) {
		return false;
	}
	for(auto it = m_Current; it != m_Tasks.end(); ++it, ++m_Current) {
		auto cache_requirements = it->get_cache_requirements();
		auto segments			= m_Cache->allocate(cache_requirements);
		if(!segments.empty() || cache_requirements.empty()) {
			++m_PendingTasks;
			// we have enough cache to run the task
			if(group) {
				group->get().run([this, task = std::move(*it), segments = std::move(segments), handler]() {
					auto metrics = task(segments, m_System, m_ComponentsCache);
					m_Cache->deallocate(segments);

					// we have to wait for new tasks to be scheduled, otherwise we could lose track
					// of the amount of locks we need to release.
					auto scoped = std::scoped_lock(m_Mutex);
					m_Metrics += metrics;
					if(--m_PendingTasks == 0) {
						m_Locks.unlock();
						if(m_Current == m_Tasks.end()) {
							m_Finished = true;
							for(auto& dependent : m_DependentTasks) {
								dependent->dependency_completed();
								if(dependent->is_ready()) {
									handler->schedule(m_System, *dependent);
								}
							}
						}
					}
				});
			} else {
				std::invoke([this, task = std::move(*it), segments = std::move(segments)]() {
					auto metrics = task(segments, m_System, m_ComponentsCache);
					m_Cache->deallocate(segments);
					m_Metrics += metrics;
				});
				--m_PendingTasks;
			}
		} else {
			if(m_PendingTasks == 0 && m_Current == it) {
				m_Locks.unlock();
			}
			m_Current = it;
			return false;
		}
	}
	m_Current = m_Tasks.end();
	if(!group) {
		m_Locks.unlock();
		m_Finished = true;
		for(auto& dependent : m_DependentTasks) {
			dependent->dependency_completed();
			if(dependent->is_ready()) {
				handler->schedule(m_System, *dependent);
			}
		}
	}
	return true;
}
}	 // namespace psl::ecs::details
