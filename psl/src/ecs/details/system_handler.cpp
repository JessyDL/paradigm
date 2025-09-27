#include "psl/ecs/details/system_handler.hpp"
#include "psl/ecs/state.hpp"
#include <tbb/flow_graph.h>
#include <tbb/rw_mutex.h>

#include "Tracy/tracy.hpp"

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

#include <iostream>

namespace psl::ecs::details {

system_handler_t::system_handler_t(size_t cache_size)
	: m_SystemScheduler(m_FilteringTasks, cache_size, std::hardware_destructive_interference_size),
	  m_Arena(tbb::task_arena::automatic), m_Cache(cache_size, std::hardware_destructive_interference_size) {}
system_handler_t ::~system_handler_t() = default;

void system_handler_t::rebuild_graph(psl::array<system_task_t>& systems, state_info_t info) {
	ZoneScoped;
	m_SystemContainers.clear();
	m_FilterResults.clear();
	m_SystemContainers.reserve(systems.size());
	m_RunningSystems.clear();
	m_ComponentLocks.clear();
	auto& all_filters = info.filter_handler->get_filter_results();

	std::unordered_map<filter_id_t, std::vector<std::pair<system_id_t, size_t>>> used_filters {};
	used_filters.reserve(all_filters.size());
	std::vector<size_t> filterless_systems {};
	size_t system_index = 0;
	for(auto& system : systems) {
		auto& container = m_SystemContainers.emplace_back(system.id, system.system, system.filters.size());
		for(auto filter_id : system.filters) {
			used_filters[filter_id].emplace_back(system.id, system_index);
		}

		auto pack = system.system->create_pack();
		std::vector<component_lock_instance_t> locks {};
		for(auto const& entry : pack) {
			auto bindings = entry.get_bindings();
			for(auto const& binding : bindings) {
				auto lock_it = m_ComponentLocks.find(binding.id);
				if(lock_it == m_ComponentLocks.end()) {
					auto [it, success] = m_ComponentLocks.emplace(binding.id, component_lock_t {});
					lock_it			   = it;
					psl_assert(success, "Failed to insert a new component lock");
				}
				locks.emplace_back(&lock_it->second, system.id, !binding.is_read_only, !binding.is_indirect);
			}
		}

		if(system.filters.empty()) {
			filterless_systems.push_back(system_index);
		}

		m_RunningSystems.emplace(system.id,
								 std::make_unique<system_invocable_task_group_t>(
								   system.id,
								   system.system,
								   std::move(component_lock_instance_group_t(locks, m_ComponentLockMutex)),
								   &m_Cache));
		++system_index;
	}

	for(auto& system : systems) {
		std::vector<system_invocable_task_group_t*> dependencies {};
		dependencies.reserve(system.dependencies.size());
		for(auto dependency : system.dependencies) {
			auto it = m_RunningSystems.find(dependency);
			dependencies.push_back(it->second.get());
		}
		auto running_it = m_RunningSystems.find(system.id);
		running_it->second->set_dependencies(std::move(dependencies));
	}

	m_SystemScheduler.m_RemainingTasks = m_SystemContainers.size();

	m_FilterResults.resize(used_filters.size());
	auto filter_result_it = m_FilterResults.begin();

	for(auto const& [id, systems] : used_filters) {
		auto filter_it =
		  std::find_if(all_filters.begin(), all_filters.end(), [id](const auto& filter) { return filter.id == id; });
		psl_assert(filter_it != all_filters.end(), "Filter id {} was not found in the provided filters", id);
		auto& filter	  = *filter_it;
		*filter_result_it = filter_task_result_t {.container = &*filter_it, .workers = 1, .id = id};
		++filter_result_it;
	}

	for(auto i = 0; i < systems.size(); ++i) {
		auto& system = systems[i];
		if(system.filters.empty()) {
			continue;
		}
		auto& container = m_SystemContainers[i];
		for(auto filter_id : system.filters) {
			auto filter_it = std::find_if(all_filters.begin(), all_filters.end(), [filter_id](const auto& filter) {
				return filter.id == filter_id;
			});
			psl_assert(filter_it != all_filters.end(), "Filter id {} was not found in the provided filters", filter_id);
			container.filters.emplace_back(
			  &*std::find_if(m_FilterResults.begin(), m_FilterResults.end(), [filter_it](const auto& result) {
				  return result.id == filter_it->id;
			  }));
		}
		++system_index;
	}

	filter_result_it = m_FilterResults.begin();
	for(auto const& [id, systems] : used_filters) {
		auto filter_it =
		  std::find_if(all_filters.begin(), all_filters.end(), [id](const auto& filter) { return filter.id == id; });
		psl_assert(filter_it != all_filters.end(), "Filter id {} was not found in the provided filters", id);

		std::vector<system_task_container_t*> filter_systems {};
		for(auto& [system_id, system_index] : systems) {
			filter_systems.push_back(&m_SystemContainers[system_index]);
		}

		auto& filter = *filter_it;
		auto data =
		  filter_task_t {m_FilterState.handler,
						 filter.group->is_hierarchy_change_active() ? m_FilterState.mutated : m_FilterState.modified,
						 &filter,
						 filter_systems,
						 *filter_result_it};
		m_FilteringTasks.run([data, &system_scheduler = m_SystemScheduler]() {
			data.handler->filter(*data.filter, data.changeset);

			// determine the amount of max amount workers to use for this filter
			// this doesn't mean the system will use that many, as other filters the system depends on might
			// influence this, as well as the historical execution time of the system.
			// And lastly the user also influences this when they mark the filter as "psl::ecs::full_t",
			// which cannot be split in those cases.
			{
				static constexpr auto min_entities_per_worker = 1 << 11;
				// todo(jdl): this should be load balanced based on past performance of the system
				auto const workers	   = size_t(std::thread::hardware_concurrency());
				auto const max_workers = std::max(
				  size_t {1},
				  std::min(workers,
						   (data.filter->entities.size() - (data.filter->entities.size() % min_entities_per_worker)) /
							 min_entities_per_worker));
				data.result.workers = max_workers;
			}

			for(auto system : data.systems) {
				if(--system->pending_filters == 0) {
					system_scheduler.execute(system->id, system->system, system->filters);
				}
			}
		});

		++filter_result_it;
	}


	for(auto id : filterless_systems) {
		auto& system = m_SystemContainers[id];
		if(system.filters.empty()) {
			m_FilteringTasks.run([&system_scheduler = m_SystemScheduler, &system]() {
				system_scheduler.execute(system.id, system.system, {});
			});
		}
	}
}


auto system_handler_t::execute(state_info_t info,
							   psl::array<system_task_t>& systems,
							   std::chrono::duration<float> dTime,
							   std::chrono::duration<float> rTime,
							   size_t tick) -> psl::array<std::unique_ptr<psl::ecs::info_t>> {
	ZoneScoped;
	if(!info.state) {
		throw std::runtime_error("You need to provide a valid state to execute systems");
	}
	m_FilterState = {info.filter_handler, info.modified_entities, info.mutated_entities};
	m_SystemScheduler.prepare(*info.state,
							  dTime,
							  rTime,
							  tick,
							  &m_RunningSystems);	 // consider the calling thread the main thread for this run
	m_Arena.execute([&, this]() {
		rebuild_graph(systems, info);
		m_FilteringTasks.wait();
	});
	m_SystemScheduler.m_MainThreadExecutor.process();
	psl_assert(m_SystemScheduler.is_done(), "System scheduler is not done after execution");
	psl::array<std::unique_ptr<psl::ecs::info_t>> results {};
	results.reserve(m_SystemScheduler.m_CommandBuffers.size());
	for(auto& buffer : m_SystemScheduler.m_CommandBuffers) {
		results.push_back(std::move(buffer));
	}
	m_SystemScheduler.m_CommandBuffers.clear();
	return results;
}

std::vector<system_invocable_task_t>
system_scheduler_t::make_tasks(system_token id,
							   system_information* system,
							   std::vector<filter_task_result_t const*> const& filters) {
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
		tasks.emplace_back(id,
						   system,
						   task_packs,
						   system->is_const()
							 ? m_SharedCommandBuffer.get()
							 : m_CommandBuffers
								 .emplace_back(std::make_unique<psl::ecs::info_t>(m_SharedCommandBuffer->state,
																				  m_SharedCommandBuffer->dTime,
																				  m_SharedCommandBuffer->rTime,
																				  m_SharedCommandBuffer->tick,
																				  system->tick()))
								 ->get(),
						   m_ComponentCache);
	}
	return tasks;
}

bool try_lock(std::recursive_mutex& shared, std::vector<component_lock_instance_t>& locks) {
	auto scoped_lock = std::scoped_lock(shared);
	for(auto it = locks.begin(); it != locks.end(); ++it) {
		if(!it->try_lock()) {
			// unlock all previous locks
			for(auto rev = locks.begin(); rev != it; ++rev) {
				rev->unlock();
			}
			return false;
		}
	}
	return true;
}

void lock(std::recursive_mutex& shared, std::vector<component_lock_instance_t>& locks) {
	auto scoped_lock = std::scoped_lock(shared);
	for(auto it = locks.begin(); it != locks.cend(); ++it) {
		it->lock();
	}
}

void unlock(std::recursive_mutex& shared, std::vector<component_lock_instance_t>& locks) {
	auto scoped_lock = std::scoped_lock(shared);
	for(auto it = locks.begin(); it != locks.cend(); ++it) {
		it->unlock();
	}
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
								 std::vector<filter_task_result_t const*> const& filters) {
	auto system_it = m_RunningSystems->find(id);
	system_it->second->set_tasks(make_tasks(id, system, filters));
	if(system_it->second->is_ready()) {
		schedule(system, *system_it->second);
	}
	try_execute_pending();
	// return;
	// if(system->threading() == psl::ecs::threading::main && !m_MainThreadExecutor.is_valid_thread()) {
	//	schedule(system, std::move(tasks));
	//	return;
	// }

	// m_LockingMutex.lock();
	// if(!try_lock(locks)) {
	//	m_LockingMutex.unlock();
	//	schedule(system, std::move(tasks));
	//	return;
	// }
	// m_LockingMutex.unlock();

	// for(auto it = tasks.begin(); it != tasks.cend(); ++it) {
	//	std::vector<std::optional<memory::segment>> segments {};
	//	if(!try_allocate_cache(*it, segments)) {
	//		// failed to allocate the cache, unlock all locks and schedule the task for later execution
	//		// we assume that the subsequent tasks will at least try to allocate the same amount of memory
	//		// so we reschedule all of them
	//		// todo(jdl): these could be prioritized as we can assume space will be cleared for every task
	//		// that finishes of the same system.
	//		unlock(locks);
	//		schedule(system, std::vector<system_invocable_task_t>(it, tasks.end()));
	//		return;
	//	}
	//	if(system->threading() == psl::ecs::threading::main) {
	//		std::invoke(*it, segments);
	//		deallocate_cache(segments);
	//	} else {
	//		m_ExecutionGroup.run([scheduler = this, task = std::move(*it), segments = std::move(segments)]() {
	//			if(!task.try_lock()) {
	//				scheduler->reschedule(std::move(task));
	//			} else {
	//				std::invoke(task, segments);
	//			}
	//			scheduler->deallocate_cache(segments);
	//			scheduler->try_execute_pending();
	//		});
	//	}
	// }

	// unlock(locks);
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

void system_scheduler_t::reschedule(system_invocable_task_group_t& group) {
	m_PendingTaskGroups.push(&group);
}

void system_invocable_task_t::prepare(std::vector<std::optional<memory::segment>> const& segments) const {
	ZoneScoped;
	psl_assert(segments.size() == m_Packs.size(),
			   "The number of segments provided does not match the number of packs in the task");
	auto write_fn = [this](details::dependency_pack const& pack, memory::segment const& segment) {
		std::uintptr_t data_begin = segment.range().begin;
		auto write_fn			  = [this, entities = pack.m_Entities, &segment, &data_begin](auto& binding) {
			const auto& cInfo = m_ComponentCache->get_component_container(binding.first);
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

		auto view_fn = [this, entities = pack.m_Entities, &segment, &data_begin](auto& binding) {
			const auto& cInfo = m_ComponentCache->get_component_container(binding.first);
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
	// direct access locks can be released during the execution of the system
	// as the data is in the cache, and so no other pack has access to it
	/*for(auto& lock : m_Locks) {
		if(lock.is_direct()) {
			lock.unlock();
		}
	}*/
}
void system_invocable_task_t::execute() const {
	ZoneScoped;
	std::invoke(*m_System, *m_Info, m_Packs);
}
void system_invocable_task_t::finalize() const {
	ZoneScoped;
	/*for(auto& lock : m_Locks) {
		if(lock.is_direct()) {
			lock.lock();
		}
	}*/

	for(const auto& dep_pack : m_Packs) {
		auto bindings = dep_pack.get_bindings();
		for(auto& binding : dep_pack.m_RWBindings) {
			std::uintptr_t data = (std::uintptr_t)binding.second.data();
			m_ComponentCache->component_copy_from(dep_pack.m_Entities, binding.first, (void*)data);
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
	m_ComponentCache = &state;
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
	} else if(m_PendingTaskGroups.empty()) {
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
					task(segments);
					m_Cache->deallocate(segments);

					// we have to wait for new tasks to be scheduled, otherwise we could lose track
					// of the amount of locks we need to release.
					auto scoped = std::scoped_lock(m_Mutex);
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
					task(segments);
					m_Cache->deallocate(segments);
					if(--m_PendingTasks == 0) {
						m_Locks.unlock();
					}
				});
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
