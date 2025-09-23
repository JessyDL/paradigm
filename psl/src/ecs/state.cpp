
#include "psl/ecs/state.hpp"
#include "psl/algorithm.hpp"
#include "psl/async/async.hpp"
#include "psl/memory/region.hpp"
#include "psl/unique_ptr.hpp"
#include "tracy/Tracy.hpp"


#include <atomic>
#include <numeric>

#include <tbb/flow_graph.h>
#include <tbb/tbb.h>

using system_id_t = size_t;

class cache_storage_t {
  public:
	cache_storage_t(size_t cache_size) : m_Cache(cache_size, 128) {}
	cache_storage_t(cache_storage_t const&)			   = delete;
	cache_storage_t(cache_storage_t&&)				   = default;
	cache_storage_t& operator=(cache_storage_t const&) = delete;
	cache_storage_t& operator=(cache_storage_t&&)	   = default;

	auto allocate(size_t bytes) {
		return m_Cache.allocate(bytes);
	}

	auto deallocate(memory::segment& segment) {
		m_Cache.deallocate(segment);
	}

  private:
	memory::region m_Cache;
};

constexpr auto align(std::uintptr_t& ptr, size_t alignment) noexcept {
#pragma warning(push)
#pragma warning(disable : 4146)
	const auto orig	   = ptr;
	const auto aligned = (ptr - 1u + alignment) & -alignment;
	ptr				   = aligned;
	return aligned - orig;
#pragma warning(pop)
}


class thread_scheduler_t {
	using filter_id_t = psl::ecs::state_t::filter_id_t;

  public:
	// internal
	struct filter_result_t {
		filter_result_t(filter_id_t id, psl::ecs::state_t::filter_result* result = nullptr, bool completed = false)
			: id(id), result(result), completed(completed) {}

		filter_result_t(const filter_result_t& rhs) noexcept {
			if(this == &rhs) {
				return;
			}
			id		  = rhs.id;
			result	  = rhs.result;
			completed = rhs.completed.load();
		}

		filter_result_t(filter_result_t&& rhs) noexcept {
			if(this == &rhs) {
				return;
			}
			id			  = rhs.id;
			result		  = rhs.result;
			completed	  = rhs.completed.load();
			rhs.result	  = nullptr;
			rhs.completed = false;
		}
		filter_result_t& operator=(const filter_result_t& rhs) noexcept {
			if(this == &rhs) {
				return *this;
			}
			id		  = rhs.id;
			result	  = rhs.result;
			completed = rhs.completed.load();
			return *this;
		}
		filter_result_t& operator=(filter_result_t&& rhs) noexcept {
			if(this == &rhs) {
				return *this;
			}
			id			  = rhs.id;
			result		  = rhs.result;
			completed	  = rhs.completed.load();
			rhs.result	  = nullptr;
			rhs.completed = false;
			return *this;
		}
		filter_id_t id;
		psl::ecs::state_t::filter_result* result;
		std::atomic<bool> completed {false};
	};

	struct system_task_t {
		system_id_t id;
		std::unordered_set<filter_id_t> filters;
		psl::ecs::details::system_information* system;
		std::vector<system_id_t> dependencies;
	};

	struct component_lock_t {
		std::shared_mutex lock;
		size_t readers {0};
		system_id_t owner {};	 // in case there's an owner, anyone of the same system can share the mutex
	};

	// internal
	struct system_container_t {
		system_id_t id;
		std::vector<filter_result_t*> filters;
		std::vector<filter_id_t> filter_ids;
		psl::ecs::details::system_information* system;
		std::vector<psl::ecs::details::dependency_pack> packs;
	};

	struct state_info_t {
		psl::array<psl::ecs::entity_t> modified_entities;
		psl::array<psl::ecs::entity_t> mutated_entities;
		psl::ecs::state_t* state;
		psl::ecs::info_t* prototype_command_buffer;
	};

	// internal
	struct cache_t {
		std::vector<filter_result_t> filters;
		std::vector<system_container_t> systems;
		cache_storage_t* cache;
		std::mutex cache_mutex;
		std::unordered_map<psl::ecs::details::component_key_t, std::unique_ptr<component_lock_t>> component_locks;

		system_container_t& get_system(system_id_t system) {
			auto it = std::find_if(
			  std::begin(systems), std::end(systems), [system](const auto& sys) { return sys.id == system; });
			psl_assert(it != std::end(systems), "System id {} was not found in the provided systems", system);
			return *it;
		}

		bool is_ready(system_container_t& system) {
			return std::all_of(std::begin(system.filters), std::end(system.filters), [this](const auto& filter) {
				return filter->completed.load();
			});
		}
	};

	thread_scheduler_t(size_t cache_size = 64 * 1024 * 1024) : m_Cache(cache_size) {}

	thread_scheduler_t(thread_scheduler_t const&)			 = delete;
	thread_scheduler_t(thread_scheduler_t&&)				 = default;
	thread_scheduler_t& operator=(thread_scheduler_t const&) = delete;
	thread_scheduler_t& operator=(thread_scheduler_t&&)		 = default;

	void init_cache(cache_t& cache,
					std::vector<system_task_t> const& systems,
					psl::array<psl::ecs::state_t::filter_result>& all_filters) {
		auto get_filter = [&cache, &all_filters](filter_id_t id) -> filter_result_t* {
			auto it = std::find_if(
			  std::begin(cache.filters), std::end(cache.filters), [id](const auto& filter) { return filter.id == id; });

			if(it == cache.filters.end()) {
				throw std::runtime_error("Filter id " + std::to_string(id) + " was not found in the provided filters");
			}
			return &(*it);
		};

		std::unordered_set<filter_id_t> used_filters {};
		for(auto& system : systems) {
			used_filters.insert(system.filters.begin(), system.filters.end());
			cache.systems.push_back(
			  {.id		   = system.id,
			   .filters	   = {},
			   .filter_ids = std::vector<filter_id_t> {system.filters.begin(), system.filters.end()},
			   .system	   = system.system,
			   .packs	   = system.system->create_pack()});
		}
		cache.filters.reserve(used_filters.size());
		for(auto filter_id : used_filters) {
			auto it = std::find_if(std::begin(all_filters), std::end(all_filters), [filter_id](const auto& filter) {
				return filter.id == filter_id;
			});
			psl_assert(it != std::end(all_filters), "Filter id {} was not found in the provided filters", filter_id);
			cache.filters.emplace_back(filter_id, &(*it));

			for(auto id : it->group->get_filters()) {
				cache.component_locks.try_emplace(id, std::make_unique<component_lock_t>());
			}
		}

		for(auto& system : cache.systems) {
			for(auto filter_id : system.filter_ids) {
				system.filters.push_back(get_filter(filter_id));
			}
		}
	}

	auto execute(state_info_t info, std::vector<system_task_t>& systems) {
		tbb::task_arena arena;
		auto command_buffers = arena.execute([this, &info, &systems]() {
			tbb::task_group main_group;
			cache_t cache {};
			cache.cache = &m_Cache;
			init_cache(cache, systems, info.state->m_Filters);


			for(auto& filter : cache.filters) {
				main_group.run([&info, &filter]() {
					ZoneScopedN("Filter Apply");
					info.state->filter(*filter.result,
									   filter.result->group->is_hierarchy_change_active() ? info.mutated_entities
																						  : info.modified_entities);
					for(auto& transformation : filter.result->transformations) {
						if(transformation.should_generate) {
							transformation.entities = filter.result->entities;
							transformation.entities.erase(
							  transformation.group->transform(
								begin(transformation.entities), end(transformation.entities), *info.state),
							  end(transformation.entities));
							transformation.should_generate = false;
						}
					}
					filter.completed = true;
				});
			}
			std::vector<std::vector<std::unique_ptr<psl::ecs::info_t>>> command_buffers {};
			command_buffers.resize(cache.systems.size());
			size_t sys_index = 0;
			for(auto& sys : cache.systems) {
				main_group.run(
				  [this, &sys, &main_group, &cache, &info, &command_buffers = command_buffers[sys_index++]]() {
					  // we'll wait until the filters are ready before continuing
					  while(!cache.is_ready(sys)) {
						  std::this_thread::yield();
					  }
					  tbb::task_group prepare_cache;

					  // todo(jdl): need to set up rest of state
					  if(sys.filters.size() == 0) {
						  return;
					  }


					  std::vector<std::vector<psl::ecs::details::dependency_pack>> packs {};
					  packs.resize(sys.filters.size());
					  // for every filter in the pack, make the dependency_pack
					  prepare_cache.run_and_wait([&sys, &packs, min_entities_per_worker = m_MinEntitiesPerWorker]() {
						  ZoneScopedN("Prepare System Entities");
						  auto str = sys.system->debug_name() + "::prepare_entities";
						  ZoneName(str.data(), str.size());
						  psl_assert(sys.filters.size() == sys.system->filters().size(),
									 "System {} has a filter count mismatch ({} vs {})",
									 sys.system->debug_name(),
									 sys.filters.size(),
									 sys.system->filters().size());
						  psl_assert(sys.filters.size() == sys.system->transforms().size(),
									 "System {} has a transforms count mismatch ({} vs {})",
									 sys.system->debug_name(),
									 sys.filters.size(),
									 sys.system->transforms().size());

						  std::vector<psl::array_view<psl::ecs::entity_t>> entities;
						  for(auto i = 0u; i < sys.filters.size(); ++i) {
							  auto filter_result   = sys.filters[i]->result;
							  auto transform_group = std::next(sys.system->transforms().begin(), i)->get();


							  if(transform_group) {
								  auto transform = std::find_if(
									std::begin(filter_result->transformations),
									std::end(filter_result->transformations),
									[transform_group](const auto& data) { return *data.group == *transform_group; });
								  entities.emplace_back(transform->entities);
							  } else {
								  entities.emplace_back(filter_result->entities);
							  }
						  }

						  auto [smallest_batch, largest_batch] = std::minmax_element(
							std::begin(entities), std::end(entities), [](const auto& lhs, const auto& rhs) {
								return lhs.size() < rhs.size();
							});

						  // todo(jdl): this should be load balanced based on past performance of the system
						  auto workers	   = (size_t)std::thread::hardware_concurrency();
						  auto max_workers = std::max<size_t>(
							1u,
							std::min(workers,
									 (largest_batch->size() - (largest_batch->size() % min_entities_per_worker)) /
									   min_entities_per_worker));

						  // To guard having systems run with concurrent packs that have no data in them.
						  // Doing so would seem counter-intuitive to users
						  while((float)smallest_batch->size() / (float)max_workers < 1.0f && max_workers > 1) {
							  --max_workers;
						  }
						  workers = max_workers;

						  auto prototype_pack = sys.system->create_pack();

						  for(auto i = 0u; i < prototype_pack.size(); ++i) {
							  if(prototype_pack[i].is_partial_pack() && workers > 1) {
								  packs[i].reserve(workers);
								  auto batch_size = entities[i].size() / workers;
								  size_t processed {0};
								  for(size_t u = 0; u < workers - 1; ++u) {
									  packs[i].emplace_back(prototype_pack[i].from_entities(
										entities[i].slice(processed, processed + batch_size)));
									  processed += batch_size;
								  }
								  packs[i].emplace_back(prototype_pack[i].from_entities(
									entities[i].slice(processed, sys.filters[i]->result->entities.size())));
							  } else {
								  packs[i].emplace_back(prototype_pack[i].from_entities(entities[i]));
							  }
						  }
					  });


					  size_t max_tasks =
						std::max_element(std::begin(packs), std::end(packs), [](const auto& lhs, const auto& rhs) {
							return lhs.size() < rhs.size();
						})->size();

					  std::vector<size_t> pack_sizes {};
					  pack_sizes.reserve(packs.size());
					  for(auto i = 0u; i < max_tasks; ++i) {
						  size_t total_size {0};
						  for(auto& pack : packs) {
							  auto pack_index = pack.size() == 1 ? 0 : i;
							  // todo(jdl): we can share the first entries of the pack if they are the same
							  // this will save memory and cache pressure
							  // i.e. a pack of 10, 1, 10, 1
							  // could have the pack entries of size 1 share with the other 9 invocations that will
							  // happen
							  total_size += sizeof(psl::ecs::entity_t) * pack[pack_index].entities();
							  total_size += align(total_size, pack[pack_index].align_of_first_binding());
							  total_size += pack[pack_index].bindings_total_size();
						  }
						  pack_sizes.push_back(total_size);
					  }


					  command_buffers.resize(max_tasks);
					  prepare_cache.run_and_wait(
						[max_tasks, &packs, &sys, &info, &pack_sizes, &cache, &command_buffers]() {
							// run system
							tbb::parallel_for(
							  size_t {0},
							  max_tasks,
							  size_t {1},
							  [&packs, &sys, &info, &pack_sizes, &cache, &command_buffers](size_t index) {
								  std::vector<psl::ecs::details::dependency_pack> task_packs {};
								  for(auto& pack : packs) {
									  if(pack.size() == 1) {
										  task_packs.push_back(pack[0]);
									  } else {
										  task_packs.push_back(pack[index]);
									  }
								  }
								  std::vector<std::shared_lock<std::shared_mutex>> locks;
								  std::vector<component_lock_t*> components;
								  std::unordered_set<psl::ecs::details::component_key_t> component_keys;
								  for(auto const& filter : sys.filters) {
									  for(auto const& component_id : filter->result->group->get_filters()) {
										  component_keys.insert(component_id);
									  }
								  }
								  locks.reserve(component_keys.size());
								  components.reserve(component_keys.size());
								  for(auto component_id : component_keys) {
									  components.emplace_back(cache.component_locks.find(component_id)->second.get());
									  locks.emplace_back(components.back()->lock, std::defer_lock);
								  }

								  auto try_lock_shared = [&components, &locks, &sys]() {
									  bool all_locked = false;
									  if(std::all_of(
										   components.begin(), components.end(), [&sys](component_lock_t* ptr) {
											   return ptr->owner == sys.id || ptr->owner == 0;
										   })) {
										  for(auto it = locks.begin(); it != locks.end(); ++it) {
											  if(!it->try_lock()) {
												  // unroll
												  for(auto unroll_it = locks.begin(); unroll_it != it; ++unroll_it) {
													  if(unroll_it->owns_lock()) {
														  unroll_it->unlock();
													  }
												  }
												  return false;
											  }
										  }

										  for(auto& component : components) {
											  component->owner = sys.id;
											  ++component->readers;
										  }
										  return true;
									  }
									  return false;
								  };

								  auto unlock_shared = [&components, &locks, &sys]() {
									  for(auto& component : components) {
										  --component->readers;
										  if(component->readers == 0) {
											  component->owner = 0;
										  }
									  }
									  for(auto it = locks.begin(); it != locks.end(); ++it) {
										  it->unlock();
									  }
								  };

								  auto const bytes_needed = pack_sizes[index];
								  memory::segment segment {};
								  // write the data into the cache
								  if(bytes_needed > 0) {
									  std::unique_lock lock(cache.cache_mutex);
									  while(!try_lock_shared()) {
										  lock.unlock();
										  std::this_thread::yield();
										  lock.lock();
									  }
									  std::optional<memory::segment> segment_opt = cache.cache->allocate(bytes_needed);
									  while(!segment_opt) {
										  lock.unlock();
										  std::this_thread::yield();
										  lock.lock();
										  segment_opt = cache.cache->allocate(bytes_needed);
									  };
									  lock.unlock();
									  ZoneScopedN("Prepare Bindings");
									  auto str = sys.system->debug_name() + "::prepare_bindings";
									  ZoneName(str.data(), str.size());
									  segment = *segment_opt;

									  auto offset = segment.range().begin;
									  for(auto& pack : task_packs) {
										  offset += info.state->prepare_bindings2(pack.m_Entities, (void*)offset, pack);
									  }
									  psl_assert(offset - segment.range().begin <= bytes_needed,
												 "Overallocated (used {} of {})",
												 offset - segment.range().begin,
												 bytes_needed);
								  }


								  // todo(jdl): should be hoisted out
								  psl::ecs::info_t shared_command_bufer(*info.state,
																		info.prototype_command_buffer->dTime,
																		info.prototype_command_buffer->rTime,
																		info.prototype_command_buffer->tick,
																		sys.system->tick());

								  psl::ecs::info_t* command_buffer = nullptr;
								  if(!sys.system->is_const()) {
									  command_buffers[index] =
										std::make_unique<psl::ecs::info_t>(*info.state,
																		   info.prototype_command_buffer->dTime,
																		   info.prototype_command_buffer->rTime,
																		   info.prototype_command_buffer->tick,
																		   sys.system->tick());
									  command_buffer = command_buffers[index].get();
								  } else {
									  command_buffer = &shared_command_bufer;
								  }
								  {
									  ZoneScopedN("Invoke System");
									  auto str = sys.system->debug_name() + "::invoke_system";
									  ZoneName(str.data(), str.size());
									  sys.system->operator()(*command_buffer, task_packs);
								  }

								  {
									  std::unique_lock lock(cache.cache_mutex);
									  ZoneScopedN("Copy Data Back");
									  auto str = sys.system->debug_name() + "::finalize";
									  ZoneName(str.data(), str.size());
									  for(const auto& dep_pack : task_packs) {
										  for(auto& binding : dep_pack.m_RWBindings) {
											  std::uintptr_t data = (std::uintptr_t)binding.second.data();
											  info.state->component_copy_from(
												dep_pack.m_Entities, binding.first, (void*)data);
										  }
									  }
									  if(bytes_needed > 0) {
										  cache.cache->deallocate(segment);
										  unlock_shared();
									  }
								  }
							  });
						});
				  });
			}
			main_group.wait();

			std::vector<std::unique_ptr<psl::ecs::info_t>> flattened_command_buffers {};

			for(auto& vec : command_buffers) {
				for(auto& cmd : vec) {
					if(cmd) {
						flattened_command_buffers.push_back(std::move(cmd));
					}
				}
			}

			return flattened_command_buffers;
		});

		return command_buffers;
	};

	size_t m_MinEntitiesPerWorker {2 << 9};
	cache_storage_t m_Cache;
};

using namespace psl::ecs;

using psl::ecs::details::component_key_t;

state_t::state_t(size_t workers, size_t cache_size, entity_t::size_type min_entities_per_worker)
	: details::entity_relationship_handler_t::entity_relationship_handler_t(), entity_container_t::entity_container_t(),
	  details::components_cache_t::components_cache_t(), m_Cache(cache_size),
	  m_Scheduler(
		/*new psl::async::scheduler((workers == 0) ? std::nullopt : std::optional {workers}, "ECS Worker")*/ nullptr),
	  m_MinEntitiesPerWorker(min_entities_per_worker), m_ThreadScheduler(std::make_unique<thread_scheduler_t>()) {
	m_SystemGroups.emplace(0, psl::array<details::system_token> {});
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	create_storage<entity_relationship_data_t>();
#endif
}

state_t::~state_t() = default;


psl::array<psl::array<details::dependency_pack>> slice(psl::array<details::dependency_pack>& source,
													   size_t workers = std::numeric_limits<size_t>::max(),
													   entity_t::size_type min_entities_per_worker = 1024) {
	psl::array<psl::array<details::dependency_pack>> packs {};

	if(source.size() == 0)
		return packs;
	ZoneScoped;

	auto [smallest_batch, largest_batch] =
	  std::minmax_element(std::begin(source), std::end(source), [](const auto& lhs, const auto& rhs) {
		  return lhs.entities() < rhs.entities();
	  });
	workers = std::min<size_t>(workers, std::thread::hardware_concurrency());
	auto max_workers =
	  std::max<size_t>(1u,
					   std::min(workers,
								(largest_batch->entities() - (largest_batch->entities() % min_entities_per_worker)) /
								  min_entities_per_worker));

	// To guard having systems run with concurrent packs that have no data in them.
	// Doing so would seem counter-intuitive to users
	while((float)smallest_batch->entities() / (float)max_workers < 1.0f && max_workers > 1) {
		--max_workers;
	}
	workers = max_workers;

	packs.resize(workers);
	for(auto& dep_pack : source) {
		if(dep_pack.is_partial_pack()) {
			auto batch_size = dep_pack.entities() / workers;
			size_t processed {0};
			for(size_t i = 0; i < workers - 1; ++i) {
				packs[i].emplace_back(dep_pack.slice(processed, processed + batch_size));
				processed += batch_size;
			}
			packs[packs.size() - 1].emplace_back(dep_pack.slice(processed, dep_pack.entities()));
		} else	  // if packs cannot be split, then emplace the 'full' data
		{
			for(size_t i = 0; i < workers; ++i) packs[i].emplace_back(dep_pack);
		}
	}
	return packs;
}


void state_t::init_thread_data_pack(thread_data_pack& datapack, details::system_information& information) const {
	auto pack = information.create_pack();
	datapack.is_partial_pack =
	  std::any_of(std::begin(pack), std::end(pack), [](const auto& dep_pack) { return dep_pack.is_partial_pack(); });
	for(auto& p : pack) {
		datapack.entries.push_back({.dep_pack = p, .entities = {}, .cache_size = 0});
	}

	auto filter_groups	  = information.filters();
	auto transform_groups = information.transforms();

	auto filter_it	  = begin(filter_groups);
	auto transform_it = begin(transform_groups);

	for(auto& entry : datapack.entries) {
		auto group_it = std::find_if(begin(m_Filters), end(m_Filters), [filter_it](const auto& data) {
			return data.group && *data.group == **filter_it;
		});
		if(*transform_it) {
			ZoneScopedN("Transform Apply");
			auto transform = std::find_if(begin(group_it->transformations),
										  end(group_it->transformations),
										  [transform_it](const auto& data) { return data.group == *transform_it; });

			if(transform->should_generate) {
				transform->entities = group_it->entities;
				transform->entities.erase(
				  transform->group->transform(begin(transform->entities), end(transform->entities), *this),
				  end(transform->entities));
				transform->should_generate = false;
			}
			entry.entities = transform->entities;
		} else {
			entry.entities = group_it->entities;
		}

		filter_it	 = std::next(filter_it);
		transform_it = std::next(transform_it);
	}

	auto [smallest_batch, largest_batch] =
	  std::minmax_element(std::begin(datapack.entries),
						  std::end(datapack.entries),
						  [](const auto& lhs, const auto& rhs) { return lhs.entities.size() < rhs.entities.size(); });
	auto workers	 = std::min<size_t>(m_Scheduler->workers() + 1, std::thread::hardware_concurrency());
	auto max_workers = std::max<size_t>(
	  1u,
	  std::min(workers,
			   (largest_batch->entities.size() - (largest_batch->entities.size() % m_MinEntitiesPerWorker)) /
				 m_MinEntitiesPerWorker));

	// To guard having systems run with concurrent packs that have no data in them.
	// Doing so would seem counter-intuitive to users
	while((float)smallest_batch->entities.size() / (float)max_workers < 1.0f && max_workers > 1) {
		--max_workers;
	}
	datapack.subpacks = max_workers;

	std::vector<thread_data_pack::entry> entries = std::move(datapack.entries);
	datapack.entries.clear();

	// now expand the existing datapack.entries to match the number of workers, and split the entities accordingly
	for(auto& entry : entries) {
		if(entry.dep_pack.is_partial_pack()) {
			datapack.unique_entries.push_back(datapack.entries.size());
			datapack.entry_count.push_back(datapack.subpacks);
			auto batch_size = entry.entities.size() / datapack.subpacks;
			size_t processed {0};
			for(size_t i = 0; i < datapack.subpacks; ++i) {
				datapack.entries.push_back(
				  {.dep_pack   = entry.dep_pack.slice(processed, processed + batch_size),
				   .entities   = psl::array_view<entity_t>(entry.entities.data() + processed, batch_size),
				   .cache_size = 0});
				processed += batch_size;
			}
			entry.dep_pack = entry.dep_pack.slice(processed, entry.dep_pack.entities());
			entry.entities = psl::array_view<entity_t>(entry.entities.data() + processed,
													   entry.entities.size() - processed);	  // remaining
		} else {
			datapack.unique_entries.push_back(datapack.entries.size());
			datapack.entry_count.push_back(1);
			// if packs cannot be split, then replicate the 'full' data
			datapack.entries.push_back(entry);
		}
	}
}


void state_t::calculate_cache_size(thread_data_pack& datapack, details::system_information& information) const {
	for(auto& entry : datapack.entries) {
		entry.cache_size = calculate_required_cache_size(entry.entities, entry.dep_pack);
	}
}
void state_t::prepare_bindings(thread_data_pack& datapack, details::system_information& information) {}
void state_t::invoke_system(thread_data_pack& datapack, details::system_information& information) {
	std::vector<std::unique_ptr<info_t>> infoBuffer {};
	info_t* infoBufferPtr {nullptr};
	if(information.is_const()) {
		infoBufferPtr = m_ConstInfo.get();
	} else {
		infoBuffer.reserve(datapack.subpacks);
		for(size_t i = 0; i < datapack.subpacks; ++i) {
			infoBuffer.emplace_back(
			  new info_t(*this, m_ConstInfo->dTime, m_ConstInfo->rTime, m_Tick, information.tick()));
		}
		infoBufferPtr = infoBuffer[0].get();
	}


	for(auto i = 0; i < datapack.subpacks; ++i) {
		std::vector<details::dependency_pack> packs {};
		auto e_count_it = begin(datapack.entry_count);
		for(auto entry_idx : datapack.unique_entries) {
			auto& entry = datapack.entries[entry_idx + (*e_count_it == 1) ? 1 : i];
			packs.emplace_back(entry.dep_pack);
			++e_count_it;
		}

		std::invoke(information.system(), *infoBufferPtr, packs);
		if(!information.is_const()) {
			++infoBufferPtr;
		}
	}
}

void state_t::write_system_results(thread_data_pack& datapack, details::system_information& information) {
	for(const auto& entry : datapack.entries) {
		for(auto& binding : entry.dep_pack.m_RWBindings) {
			std::uintptr_t data = (std::uintptr_t)binding.second.data();
			component_copy_from(entry.dep_pack.m_Entities, binding.first, (void*)data);
		}
	}
}

// next step is to alloc the cache and then copy the data into the packs
// lastly we invoke the systems, and then write the data back


void state_t::prepare_system(std::chrono::duration<float> dTime,
							 std::chrono::duration<float> rTime,
							 std::uintptr_t cache_offset,
							 details::system_information& information) {
	ZoneScoped;
	auto write_data = [](state_t& state, psl::array<details::dependency_pack> const& dep_packs) {
		ZoneScoped;
		for(const auto& dep_pack : dep_packs) {
			for(auto& binding : dep_pack.m_RWBindings) {
				std::uintptr_t data = (std::uintptr_t)binding.second.data();
				state.component_copy_from(dep_pack.m_Entities, binding.first, (void*)data);
			}
		}
	};

	auto pack = information.create_pack();
	bool is_partial_pack =
	  std::any_of(std::begin(pack), std::end(pack), [](const auto& dep_pack) { return dep_pack.is_partial_pack(); });


	auto filter_groups	  = information.filters();
	auto transform_groups = information.transforms();

	auto filter_it	  = begin(filter_groups);
	auto transform_it = begin(transform_groups);

	auto system_tick = information.tick();

	m_ConstInfo->dTime		 = dTime;
	m_ConstInfo->rTime		 = rTime;
	m_ConstInfo->system_tick = system_tick;

	// todo(jdl): support information.threading() == threading::par

	if(is_partial_pack) {
		for(auto& dep_pack : pack) {
			psl::array_view<entity_t> entities;
			auto group_it = std::find_if(begin(m_Filters), end(m_Filters), [filter_it](const auto& data) {
				return data.group && *data.group == **filter_it;
			});
			if(*transform_it) {
				ZoneScopedN("Transform Apply");
				auto transform = std::find_if(begin(group_it->transformations),
											  end(group_it->transformations),
											  [transform_it](const auto& data) { return data.group == *transform_it; });

				if(transform->should_generate) {
					transform->entities = group_it->entities;
					transform->entities.erase(
					  transform->group->transform(begin(transform->entities), end(transform->entities), *this),
					  end(transform->entities));
					transform->should_generate = false;
				}
				entities = transform->entities;
			} else {
				entities = group_it->entities;
			}

			filter_it	 = std::next(filter_it);
			transform_it = std::next(transform_it);
			if(entities.size() == 0)
				continue;

			cache_offset += prepare_bindings(entities, (void*)cache_offset, dep_pack);
		}

		// main thread participates, so workers + 1
		auto multi_pack = slice(pack, m_Scheduler->workers() + 1, m_MinEntitiesPerWorker);

		info_t* infoBuffer {nullptr};
		auto index = m_InfoBuffer.size();
		if(information.is_const()) {
			infoBuffer = m_ConstInfo.get();
		} else {
			for(size_t i = 0; i < std::min(m_Scheduler->workers() + 1, multi_pack.size()); ++i) {
				m_InfoBuffer.emplace_back(new info_t(*this, dTime, rTime, m_Tick, system_tick));
			}

			infoBuffer = m_InfoBuffer[index].get();
		}

		for(auto& mPack : multi_pack) {
			if(!information.is_const()) {
				infoBuffer = m_InfoBuffer[index++].get();
			}
			auto t1 = m_Scheduler->schedule([&fn	  = information.system(),
											 info_ptr = infoBuffer,
											 &mPack,
											 name = psl::string {information.debug_name()}]() {
				ZoneScopedN("System Invoke");
				ZoneName(name.data(), name.size());
				std::invoke(fn, *info_ptr, mPack);
			});
			auto t2 = m_Scheduler->schedule([this, write_data, &mPack]() { std::invoke(write_data, *this, mPack); });
			t2.after(t1);
		}
		m_Scheduler->execute();
	} else {
		for(auto& dep_pack : pack) {
			psl::array_view<entity_t> entities;
			auto group_it = std::find_if(begin(m_Filters), end(m_Filters), [filter_it](const auto& data) {
				return data.group && *data.group == **filter_it;
			});
			psl_assert(group_it != std::end(m_Filters),
					   "Could not find a matching filter for the system {} with debug name '{}'",
					   information.id().value(),
					   information.debug_name());
			if(*transform_it) {
				ZoneScopedN("Transform Apply");
				auto transform = std::find_if(begin(group_it->transformations),
											  end(group_it->transformations),
											  [transform_it](const auto& data) { return data.group == *transform_it; });
				if(transform->should_generate) {
					transform->entities = group_it->entities;
					transform->entities.erase(
					  transform->group->transform(begin(transform->entities), end(transform->entities), *this),
					  end(transform->entities));
					transform->should_generate = false;
				}
				entities = transform->entities;
			} else {
				entities = group_it->entities;
			}

			filter_it	 = std::next(filter_it);
			transform_it = std::next(transform_it);
			if(entities.size() == 0)
				continue;

			cache_offset += prepare_bindings(entities, (void*)cache_offset, dep_pack);
		}

		auto& info = information.is_const()
					   ? m_ConstInfo
					   : m_InfoBuffer.emplace_back(new info_t(*this, dTime, rTime, m_Tick, system_tick));
		{
			ZoneScopedN("System Invoke");
			ZoneName(information.debug_name().data(), information.debug_name().size());
			information.operator()(*info, pack);
		}

		write_data(*this, pack);
	}
}

void state_t::update_relationship_components() {
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	ZoneScoped;
	// we might be better off doing this in the filter loop instead.
	auto hierarchyCInfo = get_component_typed_info<entity_relationship_data_t>();
	auto update_event_component_data =
	  [this, &hierarchyCInfo](entity_t e, entity_relationship_data_t& data, hierarchy_change_event event) {
		  if((event & hierarchy_change_event::child_changed) != hierarchy_change_event::none) {
			  data.m_Children = std::make_shared<psl::array<entity_t>>(get_direct_children(e));

			  // Notify my direct children of the changes to their siblings.
			  for(auto child : *data.m_Children) {
				  if(auto dataPtr = static_cast<entity_relationship_data_t*>(
					   hierarchyCInfo->get_if(child, details::stage_range_t::ALL));
					 dataPtr) {
					  dataPtr->m_Siblings = data.m_Children;
				  } else {
					  entity_relationship_data_t data {};
					  data.m_Self	  = child;
					  data.m_Siblings = data.m_Children;
					  data.m_Parent	  = e;
					  hierarchyCInfo->add(child, &data);
				  }
			  }
		  }
		  if((event & hierarchy_change_event::reparented) != hierarchy_change_event::none) {
			  data.m_Parent = get_parent(e);
			  // only need to handle this when we unparent an entity, if the parent exists in the hierarchy then we
			  // fetch the children to set the siblings. If the parent doesn't exist yet it will set the siblings
			  // for us.
			  if(data.m_Parent == invalid_entity) {
				  data.m_Siblings->clear();
			  } else if(auto parent = static_cast<entity_relationship_data_t*>(
						  hierarchyCInfo->get_if(data.m_Parent, details::stage_range_t::ALL));
						parent) {
				  data.m_Siblings = parent->m_Children;
			  }
		  }
	  };

	auto mod_hierarchy_entities = modified_hierarchy_entities();
	auto mod_hierarchy_data		= modified_hierarchy_data();
	auto event_it				= std::begin(mod_hierarchy_data);
	for(auto ent_it = std::begin(mod_hierarchy_entities); ent_it != std::end(mod_hierarchy_entities);
		++ent_it, ++event_it) {
		auto e = *ent_it;
		if(auto dataPtr =
			 static_cast<entity_relationship_data_t*>(hierarchyCInfo->get_if(e, details::stage_range_t::ALL));
		   dataPtr) {
			update_event_component_data(e, *dataPtr, *event_it);
		} else {
			entity_relationship_data_t data {};
			data.m_Self = e;
			update_event_component_data(e, data, *event_it);
			hierarchyCInfo->add(e, &data);
		}
	}
#endif
}

void state_t::tick(std::chrono::duration<float> dTime) {
	tick(dTime, psl::array_view<system_group_t> {});
}
void state_t::tick(std::chrono::duration<float> dTime, system_group_t group) {
	tick(dTime, psl::array_view<system_group_t> {&group, 1});
}
void state_t::tick(std::chrono::duration<float> dTime, psl::array_view<system_group_t> groups) {
	ZoneScoped;
	m_LockState = 1;

	if(!m_ConstInfo) {
		m_ConstInfo = std::move(std::make_unique<info_t>(*this, dTime, dTime, m_Tick, 0));
	} else {
		m_ConstInfo->dTime = dTime;
		m_ConstInfo->rTime = dTime;
		m_ConstInfo->tick  = m_Tick;
	}

	update_relationship_components();

	// remove filters that are no longer in use
	m_Filters.erase(std::remove_if(begin(m_Filters),
								   end(m_Filters),
								   [](const filter_result& res) { return res.group.use_count() <= 1; }),
					end(m_Filters));

	auto mod_entities			= entity_container_t::modified_entities();
	auto mod_hierarchy_entities = psl::array<entity_t> {modified_hierarchy_entities()};
	std::sort(std::begin(mod_entities), std::end(mod_entities));
	std::sort(std::begin(mod_hierarchy_entities), std::end(mod_hierarchy_entities));

	psl::array<details::component_container_t*> mutated_components = apply_mutations();

	std::vector<thread_scheduler_t::system_task_t> systems_to_execute {};

	if(groups.size() == 0) {
		for(auto& system : m_SystemInformations) {
			if(m_SystemGroupIndices.find(system.id()) != std::end(m_SystemGroupIndices)) {
				continue;
			}
			auto& task	= systems_to_execute.emplace_back();
			task.id		= system.id().value();
			task.system = &system;

			for(auto& filter : system.m_Filters) {
				auto filter_it = std::find_if(begin(m_Filters), end(m_Filters), [&filter](const auto& data) {
					return data.group && *data.group == *filter;
				});
				psl_assert(filter_it != std::end(m_Filters),
						   "Could not find a matching filter for the system {} with debug name '{}'",
						   system.id().value(),
						   system.debug_name());
				task.filters.insert(filter_it->id);
			}
		}
	}


	auto command_buffers = m_ThreadScheduler->execute(
	  {
		.modified_entities		  = mod_entities,
		.mutated_entities		  = mod_hierarchy_entities,
		.state					  = this,
		.prototype_command_buffer = m_ConstInfo.get(),
	  },
	  systems_to_execute);

	/*{
		std::unordered_map<psl::ecs::details::system_information*, std::vector<psl::async::token>>
		  systems_to_prepare {};
		std::unordered_map<filter_result*, std::vector<psl::ecs::details::system_information*>> filters_to_update
	{}; if(groups.size() == 0) { for(auto& system : m_SystemInformations) {
				if(m_SystemGroupIndices.find(system.id()) != std::end(m_SystemGroupIndices)) {
					continue;
				}
				systems_to_prepare[&system] = {};
				for(auto& filter : system.m_Filters) {
					auto filter_it = std::find_if(begin(m_Filters), end(m_Filters), [&filter](const auto& data) {
						return data.group && *data.group == *filter;
					});
					if(filter_it != std::end(m_Filters)) {
						if(auto it = filters_to_update.find(&(*filter_it)); it != std::end(filters_to_update)) {
							it->second.emplace_back(&system);
						} else {
							filters_to_update[&(*filter_it)] = {&system};
						}
					}
				}
			}
		}

		auto clear_token = m_Scheduler->schedule([this]() {
			clear_modified_entities();
			clear_modified_hierarchy();
		});

		for(auto& [filterPtr, systems] : filters_to_update) {
			auto t = m_Scheduler->schedule([this, filterPtr, &mod_entities, &mod_hierarchy_entities]() {
				filter(*filterPtr,
					   filterPtr->group->is_hierarchy_change_active() ? mod_hierarchy_entities : mod_entities);
			});

			clear_token.after(t);

			for(auto* system : systems) {
				systems_to_prepare[system].emplace_back(t);
			}
		}

		for(auto& [systemPtr, tokens] : systems_to_prepare) {
			auto t = m_Scheduler->schedule(
			  [this, systemPtr, dTime]() { prepare_system(dTime, dTime, (std::uintptr_t)m_Cache.data(), *systemPtr);
	});

			t.after(clear_token);
			for(auto& token : tokens) {
				t.after(token);
			}
		}
		m_Scheduler->execute();
	}*/

	// apply filterings
	/*for(auto& filter_result : m_Filters) {
		m_Scheduler->schedule([this, &filter_result, &mod_entities, &mod_hierarchy_entities]() {
			filter(filter_result,
				   filter_result.group->is_hierarchy_change_active() ? mod_hierarchy_entities : mod_entities);
		});
	}*/

	// m_Scheduler->execute();

	clear_modified_entities();
	clear_modified_hierarchy();

	// todo: we can optimize this, and additionally the filters should be refined for the systems that we'll
	// actually use
	//
	// when ticking if the system is a group we go down an alternate pathway where we do not do any component
	// promotion or filtering, but instead we just execute the systems in the group additionally the tick value does
	// not increment. new systems can be added and removed during this tick, but that's the only shared
	// functionality between the two. as group ticks can not use advanced filtering operations they will
	// additionally not see new components until a normal tick is performed.
	auto system_indices = std::unordered_set<details::system_token>();

	// if(groups.size() == 0) {
	//	for(auto& system : m_SystemInformations) {
	//		if(m_SystemGroupIndices.find(system.id()) != std::end(m_SystemGroupIndices)) {
	//			continue;
	//		}
	//		prepare_system(dTime, dTime, (std::uintptr_t)m_Cache.data(), system);
	//	}

	process_to_be_orphans();
	components_cache_t::purge();
	//} else {
	//	// for every group, get all the systems and append them to system_indices
	//	for(auto& group : groups) {
	//		auto group_it = m_SystemGroups.find(group.m_Id);
	//		if(group_it == std::end(m_SystemGroups)) {
	//			throw std::runtime_error("The system group '" + psl::string {group.m_DebugName} +
	//									 "' was not found in the state.");
	//		}
	//		for(auto& system : group_it->second) {
	//			system_indices.insert(system);
	//		}
	//	}

	//	for(auto& system : m_SystemInformations) {
	//		psl_assert(system_indices.contains(system.id()),
	//				   "The system '{}' with debug name '{}' was not found in the system indices for the tick.",
	//				   system.id().value(),
	//				   system.debug_name());
	//		prepare_system(dTime, dTime, (std::uintptr_t)m_Cache.data(), system);
	//	}
	//}

	// we can clear the mutated component data now as we have the filtering information:
	for(auto* cInfo : mutated_components) {
		if(cInfo) {
			cInfo->clear();
		}
	}

	for(auto& info : command_buffers) {
		execute_command_buffer(*info.get());
	}
	m_InfoBuffer.clear();

	++m_Tick;

	// here we clean up the transient filters (on_add/on_combine) which need to be merged with the pre-existing
	// filters (if available) if they are available then we will look through all the systems and update the
	// filters. if not, they become the new permanent filter.
	auto transient_filters_it =
	  std::stable_partition(std::begin(m_Filters), std::end(m_Filters), [](const filter_result& data) {
		  return !data.group->is_transient();
	  });
	for(auto it = transient_filters_it; it != std::end(m_Filters); ++it) {
		if(it->group && it->group->is_transient()) {
			it->group->disable_transience();

			auto found_it = std::find_if(std::begin(m_Filters), transient_filters_it, [&it](const filter_result& data) {
				return *data.group == *it->group;
			});

			// in case an already existing filter was found we will update it the systems to the pre-existing filter
			if(found_it != std::end(m_Filters)) {
				for(auto& system : m_SystemInformations) {
					auto system_filter_it = std::find_if(std::begin(system.m_Filters),
														 std::end(system.m_Filters),
														 [&it](const auto& filter) { return *filter == *it->group; });
					if(system_filter_it != std::end(system.m_Filters)) {
						*system_filter_it = found_it->group;
					}
				}
			}
		}
	}
	// run another gc pass to remove filters that are no longer in use.
	m_Filters.erase(std::remove_if(begin(m_Filters),
								   end(m_Filters),
								   [](const filter_result& res) { return res.group.use_count() <= 1; }),
					end(m_Filters));

	if(m_NewSystemInformations.size() > 0) {
		for(auto& system : m_NewSystemInformations) m_SystemInformations.emplace_back(std::move(system));
		m_NewSystemInformations.clear();
	}
	m_LockState = 0;

	if(m_ToRevoke.size() > 0) {
		if(groups.size() == 0) {
			for(auto id : m_ToRevoke) {
				revoke(id);
			}
			m_ToRevoke.clear();
		} else {
			m_ToRevoke.erase(
			  std::remove_if(std::begin(m_ToRevoke), std::end(m_ToRevoke), [&system_indices, this](auto id) {
				  if(system_indices.contains(id)) {
					  revoke(id);
					  return true;
				  }
				  return false;
			  }));
		}
	}
}

// consider an alias feature
// ie: alias transform = position, rotation, scale components
void state_t::destroy(psl::array_view<entity_t> entities) noexcept {
	ZoneScoped;
	if(entities.size() == 0)
		return;

	components_cache_t::destroy_components(entities);
	entity_container_t::destroy(entities);
}

void state_t::destroy(entity_t entity) noexcept {
	ZoneScoped;
	components_cache_t::destroy_components(entity);
	entity_container_t::destroy(entity);
}

void state_t::reset(psl::array_view<entity_t> entities) noexcept {
	ZoneScoped;
	psl::array<entity_t> storage {};
	components_cache_t::destroy_components(entities);
}

psl::array<entity_t>::iterator state_t::filter_op(details::cached_container_entry_t const& entry,
												  psl::array<entity_t>::iterator begin,
												  psl::array<entity_t>::iterator end,
												  details::stage_range_t range) const noexcept {
	ZoneScoped;
	if(!entry.container) {
		entry.container = get_component_container(entry.key);
	}
	return (entry.container == nullptr)
			 ? begin
			 : std::partition(begin, end, [&entry, &range](entity_t e) { return entry.container->has(e, range); });
}

psl::array<entity_t>::iterator state_t::on_add_op(details::cached_container_entry_t const& entry,
												  psl::array<entity_t>::iterator begin,
												  psl::array<entity_t>::iterator end) const noexcept {
	ZoneScoped;
	if(!entry.container) {
		entry.container = get_component_container(entry.key);
	}
	return (entry.container == nullptr)
			 ? begin
			 : std::partition(begin, end, [&entry](entity_t e) { return entry.container->has_added(e); });
}

psl::array<entity_t>::iterator state_t::on_remove_op(details::cached_container_entry_t const& entry,
													 psl::array<entity_t>::iterator begin,
													 psl::array<entity_t>::iterator end) const noexcept {
	ZoneScoped;
	if(!entry.container) {
		entry.container = get_component_container(entry.key);
	}
	return (entry.container == nullptr)
			 ? begin
			 : std::partition(begin, end, [&entry](entity_t e) { return entry.container->has_removed(e); });
}

psl::array<entity_t>::iterator state_t::on_except_op(details::cached_container_entry_t const& entry,
													 psl::array<entity_t>::iterator begin,
													 psl::array<entity_t>::iterator end,
													 details::stage_range_t range) const noexcept {
	ZoneScoped;
	if(!entry.container) {
		entry.container = get_component_container(entry.key);
	}
	return (entry.container == nullptr)
			 ? end
			 : std::partition(begin, end, [&entry, &range](entity_t e) { return !entry.container->has(e, range); });
}

psl::array<entity_t>::iterator state_t::on_break_op(psl::array<details::cached_container_entry_t> const& entries,
													psl::array<entity_t>::iterator begin,
													psl::array<entity_t>::iterator end) const noexcept {
	ZoneScoped;
	for(auto& entry : entries) {
		if(entry.container == nullptr) {
			entry.container = get_component_container(entry.key);
		}
	}
	return (std::any_of(entries.begin(), entries.end(), [](const auto& cache) { return cache.container == nullptr; }))
			 ? begin
			 :
			 // for every entity, remove if...
			 std::partition(begin, end, [&entries](entity_t e) {
				 return
				   // any of them have not had an entity removed
				   !(!std::any_of(std::begin(entries),
								  std::end(entries),
								  [e](const auto& entry) { return entry.container->has_removed(e); }) ||
					 // or all of them do not have a component, or had the entity removed
					 !std::all_of(std::begin(entries), std::end(entries), [e](const auto& entry) {
						 return entry.container->has_component(e) || entry.container->has_removed(e);
					 }));
			 });
}

psl::array<entity_t>::iterator state_t::on_combine_op(psl::array<details::cached_container_entry_t> const& entries,
													  psl::array<entity_t>::iterator begin,
													  psl::array<entity_t>::iterator end) const noexcept {
	ZoneScoped;
	for(auto& entry : entries) {
		if(entry.container == nullptr) {
			entry.container = get_component_container(entry.key);
		}
	}
	//// if any of the containers are null, we cannot combine them, so we return the begin iterator
	// if(std::any_of(entries.begin(), entries.end(), [](const auto& cache) { return cache.container == nullptr; }))
	// {
	//	return begin;
	// }
	// for(auto& entry : entries) {
	//	end = entry.container->remove_if_has_not(begin, end, psl::ecs::details::stage_range_t::ALIVE);
	// }
	// return std::remove_if(begin, end, [entries](entity_t e) {
	//	return !std::any_of(
	//	  std::begin(entries), std::end(entries), [e](const auto& entry) { return entry.container->has_added(e); });
	// });
	return (std::any_of(entries.begin(), entries.end(), [](const auto& cache) { return cache.container == nullptr; }))
			 ? begin
			 : std::remove_if(begin, end, [entries](entity_t e) {
				   return !std::any_of(std::begin(entries), std::end(entries), [e](const auto& entry) {
					   return entry.container->has_added(e);
				   }) || !std::all_of(std::begin(entries), std::end(entries), [e](const auto& entry) {
					   return entry.container->has_component(e);
				   });
			   });
}


psl::array<entity_t>::iterator state_t::on_mutate_op(details::cached_container_entry_t const& entry,
													 psl::array<entity_t>::iterator begin,
													 psl::array<entity_t>::iterator end) const noexcept {
	ZoneScoped;
	if(!entry.container) {
		// contains the mutated components
		entry.container = get_component_container(entry.key);
	}
	// actual component data
	auto cInfoTarget = entry.container ? get_component_container(entry.container->component_type_info().id) : nullptr;
	psl_assert(entry.container == nullptr || cInfoTarget != entry.container,
			   "The mutation data source and destination are the same container, this means we have an incorrect "
			   "lookup happening.");
	return (entry.container == nullptr || cInfoTarget == nullptr)
			 ? begin
			 : std::partition(begin, end, [&entry, &cInfoTarget](entity_t e) {
				   return entry.container->has(e) && cInfoTarget->has(e);
			   });
}

psl::array<entity_t>::iterator state_t::on_hierarchy_op(hierarchy_change_event change,
														psl::array<entity_t>::iterator begin,
														psl::array<entity_t>::iterator end) const noexcept {
	ZoneScoped;
	if(change == hierarchy_change_event::none) {
		return begin;
	}

	return std::partition(begin, end, [change, this](entity_t e) {
		if(auto it = change_event(e); it != nullptr) {
			return (*it & change) != hierarchy_change_event::none;
		}
		return false;
	});
}

psl::array<entity_t>::iterator
state_t::on_hierarchy_with_preseed_op(hierarchy_change_event change,
									  psl::array<entity_t>::iterator begin,
									  psl::array<entity_t>::iterator end) const noexcept {
	ZoneScoped;
	if(change == hierarchy_change_event::none) {
		return begin;
	}
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	return std::partition(begin, end, [change, this](entity_t e) {
		if(auto relationship = get_relationship(e.value()); relationship) {
			if((change & hierarchy_change_event::reparented) == hierarchy_change_event::reparented &&
			   relationship->parent != invalid_entity) {
				return true;
			}
			if((change & hierarchy_change_event::child_added) == hierarchy_change_event::child_added &&
			   relationship->first_child != invalid_entity) {
				return true;
			}
		}
		return false;
	});
#else
	return begin;
#endif
}

psl::array_view<entity_t> state_t::get_source_for(filter_result const& data) const noexcept {
	ZoneScoped;
	psl_assert(data.entities.size() == 0,
			   "The filter result should not have entities yet, this is used to first-pass initialize the "
			   "filter_result container.");
	std::optional<psl::array_view<entity_t>> source;

	for(auto filter : data.group->on_mutate) {
		auto cInfo = get_component_container(filter);
		if(!cInfo) {
			return {};
		}
		if(!source || cInfo->entities().size() < source.value().size()) {
			// technically remove ops on the on_mutate should not be possible, but we'll filter for all anyway.
			source = cInfo->entities(true);
		}
	}

	for(auto filter : data.group->on_remove) {
		auto cInfo = get_component_container(filter);
		if(!cInfo) {
			return {};
		}
		if(!source || cInfo->removed_entities().size() < source.value().size()) {
			source = cInfo->removed_entities();
		}
	}
	for(auto filter : data.group->on_break) {
		auto cInfo = get_component_container(filter);
		if(!cInfo) {
			return {};
		}
		if(!source || cInfo->entities(true).size() < source.value().size()) {
			source = cInfo->entities(true);
		}
	}
	for(auto filter : data.group->on_add) {
		auto cInfo = get_component_container(filter);
		if(!cInfo) {
			return {};
		}
		if(data.group->seed_with_previous) {
			if(!source || cInfo->entities().size() < source.value().size()) {
				source = cInfo->entities();
			}
		} else {
			if(!source || cInfo->added_entities().size() < source.value().size()) {
				source = cInfo->added_entities();
			}
		}
	}
	for(auto filter : data.group->on_combine) {
		auto cInfo = get_component_container(filter);
		if(!cInfo) {
			return {};
		}
		if(!source || cInfo->entities().size() < source.value().size()) {
			source = cInfo->entities();
		}
	}

	for(auto filter : data.group->filters) {
		auto cInfo = get_component_container(filter);
		if(!cInfo) {
			return {};
		}
		if(!source || cInfo->entities().size() < source.value().size()) {
			source = cInfo->entities();
		}
	}

	return source.value_or(psl::array_view<entity_t> {});
}

psl::array<entity_t>::iterator state_t::filter(details::filter_group const& group,
											   psl::array<entity_t>::iterator begin,
											   psl::array<entity_t>::iterator end) const noexcept {
	ZoneScoped;
	auto const range = group.on_break.size() > 0 ? details::stage_range_t::ALL : details::stage_range_t::ALIVE;
	for(auto filter : group.on_mutate) {
		end = on_mutate_op(filter, begin, end);
	}

	for(auto filter : group.on_remove) {
		end = on_remove_op(filter, begin, end);
	}
	if(group.on_break.size() > 0) {
		end = on_break_op(group.on_break, begin, end);
	}

	if(group.is_transient()) {
		for(auto filter : group.on_add) {
			end = filter_op(filter, begin, end);
		}
		for(auto filter : group.on_combine) {
			end = filter_op(filter, begin, end);
		}
	} else {
		for(auto filter : group.on_add) {
			end = on_add_op(filter, begin, end);
		}
		if(group.on_combine.size() > 0) {
			end = on_combine_op(group.on_combine, begin, end);
		}
	}
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	static constexpr auto erd_key = component_key_t::generate<entity_relationship_data_t>();
#endif
	for(auto filter : group.filters) {
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
		if(filter.key == erd_key) {
			continue;
		}
#endif
		end = filter_op(filter, begin, end, range);
	}
	for(auto filter : group.except) {
		end = on_except_op(filter, begin, end, range);
	}

	return end;
}
psl::array<entity_t> state_t::get_all_relationships_unfiltered(psl::array_view<entity_t> source,
															   entity_relationship relationship) const noexcept {
	ZoneScoped;
	auto begin = std::begin(source);
	auto end   = std::end(source);

	psl::array<entity_t> relationship_entities {};
	if((relationship & entity_relationship::all_parents) == entity_relationship::all_parents) {
		for(auto it = begin; it != end; it = std::next(it)) {
			auto parents = get_all_parents(*it);
			relationship_entities.insert(std::end(relationship_entities), std::begin(parents), std::end(parents));
		}
	} else if((relationship & entity_relationship::direct_parent) == entity_relationship::direct_parent) {
		for(auto it = begin; it != end; it = std::next(it)) {
			relationship_entities.emplace_back(get_parent(it->value()));
		}
	}

	if((relationship & entity_relationship::all_children) == entity_relationship::all_children) {
		for(auto it = begin; it != end; it = std::next(it)) {
			auto children = get_all_children(*it);
			relationship_entities.insert(std::end(relationship_entities), std::begin(children), std::end(children));
		}
	} else if((relationship & entity_relationship::direct_children) == entity_relationship::direct_children) {
		for(auto it = begin; it != end; it = std::next(it)) {
			auto children = get_children(*it, true);
			relationship_entities.insert(std::end(relationship_entities), std::begin(children), std::end(children));
		}
	}

	if((relationship & entity_relationship::siblings) == entity_relationship::siblings) {
		for(auto it = begin; it != end; it = std::next(it)) {
			auto siblings = get_siblings(*it);
			relationship_entities.insert(std::end(relationship_entities), std::begin(siblings), std::end(siblings));
		}
	}

	if((relationship & entity_relationship::self) == entity_relationship::self) {
		relationship_entities.insert(std::end(relationship_entities), std::begin(source), std::end(source));
	}

	return relationship_entities;
}

void state_t::initialize_filter(filter_result& data) const noexcept {
	if(data.group->should_be_preseeded()) {
		auto source = get_source_for(data);

#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
		if(data.group->is_hierarchy_seed_with_previous()) {
			auto cpy = psl::array<entity_t>(source.begin(), source.end());
			cpy.erase(std::remove_if(cpy.begin(),
									 cpy.end(),
									 [this, &group = *data.group](entity_t e) {
										 if(auto relationship = get_relationship(e.value()); relationship) {
											 if((group.hierarchy_change & hierarchy_change_event::reparented) ==
												  hierarchy_change_event::reparented &&
												relationship->parent != invalid_entity) {
												 return false;
											 }
											 if((group.hierarchy_change & hierarchy_change_event::child_added) ==
												  hierarchy_change_event::child_added &&
												relationship->first_child != invalid_entity) {
												 return false;
											 }
										 }
										 return true;
									 }),
					  cpy.end());

			auto cached					 = data.group->hierarchy_change;
			data.group->hierarchy_change = hierarchy_change_event::none;
			filter(data, psl::array_view<entity_t>(cpy.begin(), cpy.end()));
			if(data.direct_entities.has_value()) {
				data.entities		 = std::move(data.direct_entities.value());
				data.direct_entities = std::nullopt;	// clear the direct entities as we no longer need them
			}
			data.group->hierarchy_change = cached;
		} else {
			filter(data, source);
		}
#else
		filter(data, source);
#endif
	}
}

void state_t::filter(filter_result& data, psl::array_view<entity_t> source) const noexcept {
	ZoneScoped;
	// reset the transformations state
	for(auto& transformation : data.transformations) {
		transformation.should_generate = true;
	}
	if(data.direct_entities.has_value() && data.direct_entities->size() > 0) {
		// if we have direct entities, we can use those as the source
		data.entities		 = data.direct_entities.value();
		data.direct_entities = std::nullopt;	// clear the direct entities as we no longer need them
	}

	if(source.size() == 0) {
		if(!data.group->is_transient() && data.group->clear_every_frame()) {
			data.entities.clear();
		}
	} else {
		psl::array<entity_t> result {source};

		// first do the hierarchy change events, as the parents need to satisfy the filters as well and can add to
		// the source entities we can cache the resultant query of the modified entities for subsequent filters
		auto begin = std::begin(result);
		auto end   = std::end(result);

		end = filter(*data.group, begin, end);

		if(data.group->is_hierarchy_change_active()) {
			if(data.group->is_hierarchy_seed_with_previous()) {
				end = on_hierarchy_with_preseed_op(data.group->hierarchy_change, begin, end);
			} else {
				end = on_hierarchy_op(data.group->hierarchy_change, begin, end);
			}
		}

		std::sort(begin, end);
		if(data.group->clear_every_frame() && !data.group->is_transient()) {
			result.erase(end, std::end(result));
			data.entities = std::move(result);

			// do normal operations here, we cannot save perf
			for(auto& transformation : data.transformations) {
				if(!transformation.group || !transformation.group->order_by ||
				   !transformation.group->on_condition.empty()) {
					continue;
				}

				transformation.entities = data.entities;
				transformation.entities.erase(transformation.group->transform(std::begin(transformation.entities),
																			  std::end(transformation.entities),
																			  *this),
											  std::end(transformation.entities));

				transformation.should_generate = false;
			}
		} else {
			// invoke<entity_t::size_type>([](auto... args) { std::sort(args...); }, end, std::end(result));
			//
			// todo support order_by and on_condition
			// if(false && data.transformations.size() > 0)
			//{
			//	for (auto& transformation : data.transformations)
			//	{
			//		continue;
			//		psl::array<entity_t> ordered_indices(transformation.entities.size());
			//		std::iota(std::begin(ordered_indices), std::end(ordered_indices), 0);

			//		auto zip = psl::zip(transformation.entities, transformation.indices, ordered_indices);

			//		// unwind existing entities
			//		// todo instead of sort, implement this as a sweeping swap
			//		psl::sorting::quick(std::begin(zip), std::end(zip), [](const auto& lhs, const auto& rhs)
			//			{
			//				return lhs.get<1>() < rhs.get<1>();
			//			});

			//		std::tuple<psl::array<entity_t>, psl::array<entity_t>, psl::array<entity_t>> diff_set{};

			//		// apply the normal merging operations, keeping track of index changes
			//		std::set_difference(std::begin(zip), std::end(zip), end, std::end(result),
			// special_inserter(diff_set),
			//			[](const auto& lhs, const auto& rhs)
			//			{
			//				if constexpr (std::is_same_v<decltype(lhs), const entity_t&>)
			//					return rhs.get<0>() < lhs;
			//				else
			//					return lhs.get<0>() < rhs; });

			//		transformation.entities = std::move(std::get<0>(diff_set));
			//		transformation.entities.insert(std::end(transformation.entities), begin, end);

			//		transformation.indices.resize(transformation.entities.size());
			//		std::iota(std::begin(transformation.indices), std::end(transformation.indices), 0);

			//		ordered_indices = std::move(std::get<2>(diff_set));
			//		auto size = ordered_indices.size();
			//		ordered_indices.resize(ordered_indices.size() + std::distance(begin, end));
			//		std::fill(std::next(std::begin(ordered_indices), size), std::next(std::begin(ordered_indices),
			// std::distance(begin, end)), std::numeric_limits<entity_t::size_type>::max());

			//		zip = psl::zip(transformation.entities, transformation.indices, ordered_indices);
			//		// unwind existing entities
			//		// todo instead of sort, implement this as a sweeping swap
			//		psl::sorting::quick(std::begin(zip), std::end(zip), [](const auto& lhs, const auto& rhs)
			//			{
			//				return lhs.get<2>() < rhs.get<2>();
			//			});


			//		// apply order_by and on_condition storing the index changes
			//	}
			//}
			// else
			{
				// here the following operations happen
				// - we make a difference set between the existing entities (data.entities) and the new source
				// entities (unfiltered)
				// - we then append the list of filtered source entities to the resulting difference set
				// - as both are already sorted at this point, we can use std::inplace_merge to merge the two
				//
				// If we did not do a difference set with the original source we'd have to run a std::unique on the
				// full data.entities. This could be cheaper but we'd need to benchmark it or do some napkin math
				// first. todo(jdl): do napkin math. Most likely if the filtered source is smaller than the existing
				// entities it would be worthwhile to do the post-unique instead of the difference set.

				psl::array<entity_t> source_cpy;
				if(!std::is_sorted(std::begin(source), std::end(source))) {
					source_cpy = psl::array<entity_t>(source.begin(), source.end());
					std::sort(std::begin(source_cpy), std::end(source_cpy));
					source = psl::array_view<entity_t>(source_cpy.data(), source_cpy.size());
				}
				psl::array<entity_t> diff_set {};
				std::set_difference(std::begin(data.entities),
									std::end(data.entities),
									std::begin(source),
									std::end(source),
									std::back_inserter(diff_set));
				data.entities = std::move(diff_set);

				auto size = std::size(data.entities);
				data.entities.insert(std::end(data.entities), begin, end);


				for(auto& transformation : data.transformations) {
					if(!transformation.group || !transformation.group->order_by ||
					   !transformation.group->on_condition.empty() ||
					   (data.group->relationship & entity_relationship::self) != data.group->relationship) {
						continue;
					}

					if(transformation.entities.size() != size) {
						transformation.entities =
						  psl::array<entity_t>(data.entities.begin(), data.entities.begin() + size);
					}
					transformation.entities.insert(std::end(transformation.entities), begin, end);
					transformation.group->transform(
					  std::begin(transformation.entities), std::end(transformation.entities), *this);
					transformation.should_generate = false;
				}

				std::inplace_merge(
				  std::begin(data.entities), std::next(std::begin(data.entities), size), std::end(data.entities));
			}
		}
	}

	// if we have more relationships to resolve other than ourselves we need to resolve those now.
	if((data.group->relationship & entity_relationship::self) != data.group->relationship) {
		auto relationship_entities =
		  get_all_relationships_unfiltered(psl::array_view<entity_t> {data.entities}, data.group->relationship);
		data.direct_entities = data.entities;
		std::sort(std::begin(relationship_entities), std::end(relationship_entities));
		relationship_entities.erase(std::unique(std::begin(relationship_entities), std::end(relationship_entities)),
									std::end(relationship_entities));
		relationship_entities.erase(
		  filter(*data.group, std::begin(relationship_entities), std::end(relationship_entities)),
		  std::end(relationship_entities));

		// if we don't have a self relationship we can just replace the entities with the new list, otherwise
		// we need to merge them.
		if((data.group->relationship & entity_relationship::self) != entity_relationship::self) {
			data.entities = relationship_entities;
		} else {
			const auto size = data.entities.size();
			data.entities.insert(std::end(data.entities),
								 std::make_move_iterator(std::begin(relationship_entities)),
								 std::make_move_iterator(std::end(relationship_entities)));
			std::inplace_merge(
			  std::begin(data.entities), std::next(std::begin(data.entities), size), std::end(data.entities));
			data.entities.erase(std::unique(std::begin(data.entities), std::end(data.entities)),
								std::end(data.entities));
		}
	}

	psl_assert(std::unique(std::begin(data.entities), std::end(data.entities)) == std::end(data.entities),
			   "some entities were not unique");
	psl_assert(std::all_of(std::begin(data.group->on_combine),
						   std::end(data.group->on_combine),
						   [this, &data](auto filter) {
							   auto cInfo = get_component_container(filter);
							   return std::all_of(std::begin(data.entities),
												  std::end(data.entities),
												  [filter, &cInfo](entity_t e) { return cInfo->has_storage_for(e); });
						   }),
			   "some components failed to have storage for the entities");
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	static constexpr auto erd_key = component_key_t::generate<entity_relationship_data_t>();
	if(std::any_of(std::begin(data.group->filters), std::end(data.group->filters), [](auto const& container) {
		   return container.key == erd_key;
	   })) {
		auto component_container = get_component_container(erd_key);
		for(auto e : data.entities) {
			if(!component_container->has(e)) {
				entity_relationship_data_t value {e};
				component_container->add(e, &value);
			}
		}
	}
#endif
}

size_t state_t::prepare_data(psl::array_view<entity_t> entities, void* cache, component_key_t id) const {
	ZoneScoped;
	if(entities.size() == 0)
		return 0;
	const auto& cInfo = get_component_container(id);
	psl_assert(cInfo != nullptr, "component info was null for the key {}", id);
	psl_assert(std::all_of(std::begin(entities), std::end(entities), [&cInfo](auto e) {
		psl_assert(
		  cInfo->has_storage_for(e), "component {} does not have storage for entity {}", cInfo->id().name(), e.value());
		return true;
	}));
	if((std::uintptr_t)(cache) + (cInfo->component_type_info().size * entities.size()) >
	   (std::uintptr_t)(m_Cache.data()) + m_Cache.size()) {
		throw std::runtime_error(
		  fmt::format("Cache ran out of memory, cache size {} with remaining {}, but {} additional bytes were required",
					  m_Cache.size(),
					  (std::uintptr_t)cache - (std::uintptr_t)m_Cache.data(),
					  cInfo->component_type_info().size * entities.size()));
	}
	return cInfo->copy_to(entities, cache);
}

size_t state_t::calculate_required_cache_size(psl::array_view<entity_t> entities,
											  details::dependency_pack& dep_pack) const noexcept {
	size_t required_size {sizeof(entity_t) * entities.size()};

	if(dep_pack.is_direct_access()) {
		auto calc_fn = [count = entities.size(), &required_size, this](auto const& binding) {
			const auto& cInfo = get_component_container(binding.first);
			if(cInfo->component_type_info().size > 0) {
				auto offset = align(required_size, cInfo->component_type_info().alignment);
				required_size += offset + (cInfo->component_type_info().size * count);
			}
		};

		std::for_each(std::begin(dep_pack.m_RBindings), std::end(dep_pack.m_RBindings), calc_fn);
		std::for_each(std::begin(dep_pack.m_RWBindings), std::end(dep_pack.m_RWBindings), calc_fn);
	} else {
		auto view_fn = [count = entities.size(), &required_size, this](auto const& binding) {
			const auto& cInfo = get_component_container(binding.first);
			if(cInfo->component_type_info().size > 0) {
				auto offset = align(required_size, alignof(entity_t));
				required_size += offset + (sizeof(entity_t::size_type) * count);
			}
		};
		std::for_each(std::begin(dep_pack.m_IndirectReadBindings), std::end(dep_pack.m_IndirectReadBindings), view_fn);
		std::for_each(
		  std::begin(dep_pack.m_IndirectReadWriteBindings), std::end(dep_pack.m_IndirectReadWriteBindings), view_fn);
	}
	return required_size;
}

size_t
state_t::prepare_bindings2(psl::array_view<entity_t> entities, void* cache, details::dependency_pack& dep_pack) const {
	size_t offset_start = (std::uintptr_t)cache;
	std::memcpy(cache, entities.data(), sizeof(entity_t) * entities.size());
	dep_pack.m_Entities = psl::array_view<entity_t>(
	  (entity_t*)cache, (entity_t*)((std::uintptr_t)cache + (sizeof(entity_t) * entities.size())));

	cache = (void*)((std::uintptr_t)cache + (sizeof(entity_t) * entities.size()));


	if(dep_pack.is_direct_access()) {
		// this functional handles filling in the cache with the data for the given component
		// it offsets the `cache` every invocation with the amount the previous invocation added
		auto write_fn = [entities, &cache, this](auto& binding) {
			std::uintptr_t data_begin = (std::uintptr_t)cache;
			const auto& cInfo		  = get_component_container(binding.first);
			if(cInfo->component_type_info().size > 0) {
				auto offset		= align(data_begin, cInfo->component_type_info().alignment);
				auto write_size = cInfo->copy_to(entities, (void*)(data_begin));
				cache			= (void*)((std::uintptr_t)cache + offset + write_size);
				binding.second	= psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache);
			}
		};
		std::for_each(std::begin(dep_pack.m_RBindings), std::end(dep_pack.m_RBindings), write_fn);
		std::for_each(std::begin(dep_pack.m_RWBindings), std::end(dep_pack.m_RWBindings), write_fn);
	} else {
		// this functional handles filling in the indirect offsets to the data so that we can reconstruct
		// an indirect_array_t
		auto view_fn = [entities, &cache, this](auto& binding) {
			std::uintptr_t data_begin = (std::uintptr_t)cache;
			const auto& cInfo		  = get_component_container(binding.first);
			if(cInfo->component_type_info().size > 0) {
				auto offset = align(data_begin, alignof(entity_t));
				cache		= cInfo->write_memory_location_offsets_for(entities, (entity_t::size_type*)data_begin);
				binding.second.indices = psl::array_view<entity_t::size_type>(
				  (entity_t::size_type*)data_begin, (entity_t::size_type*)((std::uintptr_t)cache + offset));
				binding.second.data = cInfo->data();
			}
		};
		std::for_each(std::begin(dep_pack.m_IndirectReadBindings), std::end(dep_pack.m_IndirectReadBindings), view_fn);
		std::for_each(
		  std::begin(dep_pack.m_IndirectReadWriteBindings), std::end(dep_pack.m_IndirectReadWriteBindings), view_fn);
	}
	return (std::uintptr_t)cache - offset_start;
}

size_t
state_t::prepare_bindings(psl::array_view<entity_t> entities, void* cache, details::dependency_pack& dep_pack) const {
	ZoneScoped;
	size_t offset_start = (std::uintptr_t)cache;
	if((std::uintptr_t)(cache) + (sizeof(entity_t) * entities.size()) >
	   (std::uintptr_t)(m_Cache.data()) + m_Cache.size()) {
		throw std::runtime_error(
		  fmt::format("Cache ran out of memory, cache size {} with remaining {}, but {} additional bytes were required",
					  m_Cache.size(),
					  (std::uintptr_t)cache - (std::uintptr_t)m_Cache.data(),
					  sizeof(entity_t) * entities.size()));
	}
	std::memcpy(cache, entities.data(), sizeof(entity_t) * entities.size());
	dep_pack.m_Entities = psl::array_view<entity_t>(
	  (entity_t*)cache, (entity_t*)((std::uintptr_t)cache + (sizeof(entity_t) * entities.size())));

	cache = (void*)((std::uintptr_t)cache + (sizeof(entity_t) * entities.size()));

	if(dep_pack.is_direct_access()) {
		// this functional handles filling in the cache with the data for the given component
		// it offsets the `cache` every invocation with the amount the previous invocation added
		auto write_fn = [entities, &cache, this](auto& binding) {
			std::uintptr_t data_begin = (std::uintptr_t)cache;
			const auto& cInfo		  = get_component_container(binding.first);
			if(cInfo->component_type_info().size > 0) {
				auto offset		= align(data_begin, cInfo->component_type_info().alignment);
				auto write_size = prepare_data(entities, (void*)data_begin, binding.first);
				cache			= (void*)((std::uintptr_t)cache + write_size + offset);
				binding.second	= psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache);
			}
		};

		std::for_each(std::begin(dep_pack.m_RBindings), std::end(dep_pack.m_RBindings), write_fn);
		std::for_each(std::begin(dep_pack.m_RWBindings), std::end(dep_pack.m_RWBindings), write_fn);
	} else {
		// this functional handles filling in the indirect offsets to the data so that we can reconstruct
		// an indirect_array_t
		auto view_fn = [entities, &cache, this](auto& binding) {
			std::uintptr_t data_begin = (std::uintptr_t)cache;
			const auto& cInfo		  = get_component_container(binding.first);
			if(cInfo->component_type_info().size > 0) {
				data_begin += align(data_begin, alignof(entity_t));
				cache = cInfo->write_memory_location_offsets_for(entities, (entity_t::size_type*)data_begin);
				binding.second.indices =
				  psl::array_view<entity_t::size_type>((entity_t::size_type*)data_begin, (entity_t::size_type*)cache);
				binding.second.data = cInfo->data();
			}
		};
		std::for_each(std::begin(dep_pack.m_IndirectReadBindings), std::end(dep_pack.m_IndirectReadBindings), view_fn);
		std::for_each(
		  std::begin(dep_pack.m_IndirectReadWriteBindings), std::end(dep_pack.m_IndirectReadWriteBindings), view_fn);
	}
	return (std::uintptr_t)cache - offset_start;
}

void state_t::execute_command_buffer(info_t& info) {
	ZoneScoped;
	auto& buffer = info.command_buffer;

	auto destroyed_entities = buffer.m_DestroyedEntities;
	auto mid =
	  std::partition(std::begin(destroyed_entities), std::end(destroyed_entities), [first = buffer.m_First](auto e) {
		  return static_cast<entity_t::size_type>(e) >= first;
	  });

	psl::sparse_array<entity_t::size_type, entity_t::size_type> remapped_entities;
	if(buffer.m_Entities.size() > 0) {
		psl::array<entity_t> added_entities;
		std::set_difference(buffer.m_Entities.data(),
							buffer.m_Entities.data() + buffer.m_Entities.size(),
							buffer.m_DestroyedEntities.data(),
							buffer.m_DestroyedEntities.data() + buffer.m_DestroyedEntities.size(),
							std::back_inserter(added_entities));

		auto new_entities = create(added_entities.size());
		psl_assert(new_entities.size() == added_entities.size(), "new entities size should match added entities size");
		auto new_entities_it = std::begin(new_entities);
		for(auto e : added_entities) {
			remapped_entities.insert(e.value(), new_entities_it->value());
			++new_entities_it;
		}
		modify_entities(added_entities);
	}

	components_cache_t::execute_command_buffer(info, remapped_entities);
	auto span = buffer.m_ModifiedEntities.indices();
	psl::array_view<entity_t> indices {const_cast<entity_t*>(reinterpret_cast<entity_t const*>(span.data())),
									   span.size()};
	modify_entities(indices);

	if(mid != std::end(destroyed_entities)) {
		destroy(
		  psl::array_view<entity_t> {&*mid, static_cast<size_t>(std::distance(mid, std::end(destroyed_entities)))});
	}
}

void state_t::clear(bool release_memory) noexcept {
	components_cache_t::clear(release_memory);
	entity_relationship_handler_t::clear();
	entity_container_t::clear();

	m_Tick = 0;
	m_SystemInformations.clear();
	m_NewSystemInformations.clear();
	m_Filters.clear();
	m_LockState = 0;
	m_ToRevoke.clear();
	m_SystemGroupCounter = 1;
	m_SystemGroups.clear();
	m_SystemGroupIndices.clear();
	m_SystemGroups.emplace(0, psl::array<details::system_token> {});
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	create_storage<entity_relationship_data_t>();
#endif
}
