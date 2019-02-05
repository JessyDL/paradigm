#include "ecs/state.h"
#include "enumerate.h"
#include "assertions.h"
#include "ecs/command_buffer.h"
#include <future>
#include "task.h"
#include "platform_def.h"
#include "math/math.hpp"
using namespace core::ecs;

details::owner_dependency_pack details::owner_dependency_pack::slice(size_t begin, size_t end) const noexcept
{
	owner_dependency_pack cpy{*this};

	cpy.m_Entities = cpy.m_Entities.slice(begin, end);

	for(auto& binding : cpy.m_RBindings)
	{
		auto size = cpy.m_Sizes[binding.first];

		std::uintptr_t begin_mem = (std::uintptr_t)binding.second.data() + (begin * size);
		std::uintptr_t end_mem   = (std::uintptr_t)binding.second.data() + (end * size);
		binding.second = psl::array_view<std::uintptr_t>{(std::uintptr_t*)begin_mem, (std::uintptr_t*)end_mem};
	}
	for(auto& binding : cpy.m_RWBindings)
	{
		auto size = cpy.m_Sizes[binding.first];
		// binding.second = binding.second.slice(size * begin, size * end);
		std::uintptr_t begin_mem = (std::uintptr_t)binding.second.data() + (begin * size);
		std::uintptr_t end_mem   = (std::uintptr_t)binding.second.data() + (end * size);
		binding.second = psl::array_view<std::uintptr_t>{(std::uintptr_t*)begin_mem, (std::uintptr_t*)end_mem};
	}
	return cpy;
}

state::state() : m_Scheduler(new psl::async::scheduler()) {}

state::~state() { delete(m_Scheduler); }

void state::destroy_immediate(psl::array_view<entity> entities) noexcept
{
	PROFILE_SCOPE(core::profiler)
		if(entities.size() == 0) return;

	psl::bytell_map<component_key_t, std::vector<entity>> erased_entities;
	psl::bytell_map<component_key_t, std::vector<uint64_t>> erased_ids;
	core::profiler.scope_begin("erase entities");
	for(const auto& e : entities)
	{
		if(auto eMapIt = m_EntityMap.find(e); eMapIt != std::end(m_EntityMap))
		{
			for(const auto&[type, index] : eMapIt->second)
			{
				erased_entities[type].emplace_back(e);
				erased_ids[type].emplace_back(index);
			}
			m_EntityMap.erase(eMapIt);
		}
	}
	core::profiler.scope_end();

	core::profiler.scope_begin("erase IDs");
	for(auto& c : erased_ids)
	{
		if(const auto& cMapIt = m_Components.find(c.first); cMapIt != std::end(m_Components))
		{
			if(cMapIt->second.is_tag()) continue;
			if(c.second.size() > 64)
			{
				std::sort(std::begin(c.second), std::end(c.second));
				auto index = std::begin(c.second);
				auto range_start = index;
				const auto end = std::prev(std::end(c.second), 1);
				while(index != end)
				{
					auto next = std::next(index, 1);
					if(*index + 1 != *next)
					{
						cMapIt->second.generator.destroy(*range_start, std::distance(range_start, next));
						range_start = next;
					}
					index = next;
				}
				cMapIt->second.generator.destroy(*range_start, std::distance(range_start, std::end(c.second)));
			}
			else
			{
				for(auto id : c.second) cMapIt->second.generator.destroy(id);
			}
		}
	}
	core::profiler.scope_end();

	core::profiler.scope_begin("erase components");
	for(auto& c : erased_entities)
	{
		if(const auto& cMapIt = m_Components.find(c.first); cMapIt != std::end(m_Components))
		{
			std::sort(std::begin(c.second), std::end(c.second));
			auto ib = std::begin(c.second);
			auto iter = std::remove_if(std::begin(cMapIt->second.entities), std::end(cMapIt->second.entities),
									   [&ib, &c](entity x) -> bool
									   {
										   while(ib != std::end(c.second) && *ib < x) ++ib;
										   return (ib != std::end(c.second) && *ib == x);
									   });

			cMapIt->second.entities.erase(iter, cMapIt->second.entities.end());
		}
	}
	core::profiler.scope_end();
	m_StateChange[(m_Tick + 1) % 2].removed_entities.insert(std::end(m_StateChange[(m_Tick + 1) % 2].removed_entities),
															std::begin(entities), std::end(entities));
}


void state::destroy_immediate(component_key_t key, psl::array_view<entity> entities) noexcept
{
	if(entities.size() == 0) return;
	auto compIt = m_Components.find(key);
	if(compIt == std::end(m_Components)) return;

	destroy_component_generator_ids(compIt->second, entities);
}

void state::destroy(psl::array_view<entity> entities) noexcept
{
	details::key_value_container_t<component_key_t, std::vector<entity>> map;

	for(auto e : entities)
	{
		if(auto eIt = m_EntityMap.find(e); eIt != std::end(m_EntityMap))
		{
			for(const auto& pair : eIt->second)
			{
				map[pair.first].emplace_back(e);
			}
		}
	}

	for(const auto& [key, entities] : map)
		remove_components({key}, entities);

	m_StateChange[(m_Tick + 1) % 2].removed_entities.insert(std::end(m_StateChange[(m_Tick + 1) % 2].removed_entities), std::begin(entities), std::end(entities));
}

void state::destroy(entity e) noexcept { destroy(psl::array_view<entity>{&e, &e + 1}); }

size_t state::prepare_data(psl::array_view<entity> entities, memory::raw_region& cache, size_t cache_offset,
						   component_key_t id, size_t element_size) const noexcept
{
	const auto& mem_pair	  = m_Components.find(id);
	std::uintptr_t data_begin = cache_offset;

	for(const auto& e : entities)
	{
		auto eMapIt  = m_EntityMap.find(e);
		auto foundIt = std::find_if(eMapIt->second.begin(), eMapIt->second.end(),
									[&id](const std::pair<component_key_t, size_t>& pair) { return pair.first == id; });

		auto index = foundIt->second;
		void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + element_size * index);
		std::memcpy((void*)cache_offset, loc, element_size);
		cache_offset += element_size;
	}


	return cache_offset - data_begin;
}

size_t state::prepare_bindings(psl::array_view<entity> entities, memory::raw_region& cache, size_t cache_offset,
							   details::owner_dependency_pack& dep_pack) const noexcept
{
	PROFILE_SCOPE(core::profiler);
	size_t offset_start = cache_offset;
	for(auto& binding : dep_pack.m_RBindings)
	{
		std::uintptr_t data_begin = cache_offset;
		cache_offset += prepare_data(entities, cache, cache_offset, binding.first, dep_pack.m_Sizes[binding.first]);
		binding.second = psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache_offset);
	}
	for(auto& binding : dep_pack.m_RWBindings)
	{
		std::uintptr_t data_begin = cache_offset;
		cache_offset += prepare_data(entities, cache, cache_offset, binding.first, dep_pack.m_Sizes[binding.first]);
		binding.second = psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache_offset);
	}
	return cache_offset - offset_start;
}


struct workload_pack
{
	workload_pack(std::vector<entity>&& entities) : entities(entities){};

	void split(size_t count)
	{
		count				= std::max(count, size_t{1});
		size_t batch_size   = entities.size() / count;
		size_t dispatched_n = 0;
		count				= (batch_size <= 2) ? 1 : count;
		for(auto i = 0; i < count - 1; ++i)
		{
			split_entities.emplace_back(
				psl::array_view<entity>{std::next(std::begin(entities), dispatched_n), batch_size});
			dispatched_n += batch_size;
		}

		split_entities.emplace_back(
			psl::array_view<entity>{std::next(std::begin(entities), dispatched_n), entities.size() - dispatched_n});
	}

	std::vector<entity> entities;
	std::vector<psl::array_view<entity>> split_entities;
};

std::vector<std::vector<details::owner_dependency_pack>> slice(std::vector<details::owner_dependency_pack>& source,
															   size_t count)
{
	std::vector<std::vector<details::owner_dependency_pack>> res;

	bool can_split = std::any_of(std::begin(source), std::end(source), [count](const auto& dep_pack) {
		return dep_pack.allow_partial() && dep_pack.entities() > count;
	});
	count		   = (can_split) ? count : 1;
	res.resize(count);
	for(auto& dep_pack : source)
	{
		if(dep_pack.allow_partial() && dep_pack.entities() > count)
		{
			auto batch_size  = dep_pack.entities() / count;
			size_t processed = 0_sz;
			for(auto i = 0; i < count - 1; ++i)
			{
				res[i].emplace_back(dep_pack.slice(processed, processed + batch_size));
				processed += batch_size;
			}
			res[res.size() - 1].emplace_back(dep_pack.slice(processed, dep_pack.entities()));
		}
		else
		{
			for(auto i = 0; i < count; ++i) res[i].emplace_back(dep_pack);
		}
	}
	return res;
}


std::vector<command_buffer> state::prepare_system(std::chrono::duration<float> dTime,
												  std::chrono::duration<float> rTime, memory::raw_region& cache,
												  size_t cache_offset, details::system_information& system)
{
	auto write_data = [](core::ecs::state& state, std::vector<details::owner_dependency_pack> dep_packs) {
		for(const auto& dep_pack : dep_packs)
		{
			for(auto& binding : dep_pack.m_RWBindings)
			{
				const size_t size   = dep_pack.m_Sizes.at(binding.first);
				std::uintptr_t data = (std::uintptr_t)binding.second.data();
				state.set(dep_pack.m_Entities, (void*)data, size, binding.first);
			}
		}
	};
	// design:
	// 3 types of execution policies:
	//		- sequential/parallel & full packs only: only one context can be active at a given time
	//		- parallel & some/full partial packs: true parallel multi-context system
	//		- sequential & some/full partial packs: only one context can be active, but the system can be spread out and
	// fill smaller holes of context downtime
	//
	// There is one more policy type, main_only. This forces the system to run on the thread that invokes
	// ecs::state::tick. This can be used in scenarios where a system needs to touch global state and either read it, or
	// mutate it without using mutexes. heuristics:
	//   should prefer scheduling full systems first, followed by sequential. These will be the blockers for high
	//   contested components. and are antithethical to parallelization. The next heuristic should take into account
	//   highly contested components and try to get them through the funnel faster. Components are contested when there
	//   is a RW lock on them. Bigger lists of them increase the heuristic value. Lastly historical timing data should
	//   be tracked for the average time per entity a given system takes to execute. Preffering to schedule intensive
	//   blocking systems first using estimated times of execution.
	PROFILE_SCOPE(core::profiler);

	auto pack = std::invoke(system.pack_generator);
	bool has_partial =
		std::any_of(std::begin(pack), std::end(pack), [](const auto& dep_pack) { return dep_pack.allow_partial(); });

	std::vector<command_buffer> commands;
	if(has_partial && system.threading == threading::par)
	{
		for(auto& dep_pack : pack)
		{
			core::profiler.scope_begin("filtering");
			auto entities = filter(dep_pack);
			core::profiler.scope_end();
			if(entities.size() == 0) continue;
			core::profiler.scope_begin("copy");
			std::memcpy((void*)cache_offset, entities.data(), sizeof(entity) * entities.size());
			dep_pack.m_Entities = psl::array_view<core::ecs::entity>(
				(entity*)cache_offset, (entity*)(cache_offset + sizeof(entity) * entities.size()));

			cache_offset += sizeof(entity) * entities.size();

			cache_offset += prepare_bindings(entities, cache, cache_offset, dep_pack);
			core::profiler.scope_end();
		}

		auto multi_pack = slice(pack, m_Scheduler->workers());

		std::vector<std::future<command_buffer>> future_commands;

		for(auto& mPack : multi_pack)
		{
			auto t1 = m_Scheduler->schedule(system.invocable, *this, dTime, rTime, mPack);

			auto t2 = m_Scheduler->schedule(write_data, *this, mPack);

			m_Scheduler->dependency(t2.first, t1.first);

			future_commands.emplace_back(std::move(t1.second));
		}

		m_Scheduler->execute().wait();
		for(auto& fCommands : future_commands)
		{
			if(!fCommands.valid()) fCommands.wait();
			auto cmd = fCommands.get();
			execute_command_buffer(cmd);
			// commands.emplace_back(fCommands.get());
		}
	}
	else
	{
		for(auto& dep_pack : pack)
		{
			core::profiler.scope_begin("filtering");
			auto entities = filter(dep_pack);
			core::profiler.scope_end();
			if(entities.size() == 0) continue;
			core::profiler.scope_begin("copy");
			std::memcpy((void*)cache_offset, entities.data(), sizeof(entity) * entities.size());
			dep_pack.m_Entities = psl::array_view<core::ecs::entity>(
				(entity*)cache_offset, (entity*)(cache_offset + sizeof(entity) * entities.size()));

			cache_offset += sizeof(entity) * entities.size();

			cache_offset += prepare_bindings(entities, cache, cache_offset, dep_pack);
			core::profiler.scope_end();
		}

		auto cmd(std::move(std::invoke(system.invocable, *this, dTime, rTime, pack)));

		write_data(*this, pack);
		execute_command_buffer(cmd);
	}

	return commands;
}

void state::tick(std::chrono::duration<float> dTime)
{
	PROFILE_SCOPE(core::profiler)
	++m_Tick;

	/*std::vector<std::vector<std::future<std::vector<entity>>>> filter_results;
	for(const auto& system : m_SystemInformations)
	{
		auto& vec = filter_results.emplace_back();
		auto packs = std::invoke(system.pack_generator);

		for(const auto& pack : packs)
		{
			vec.emplace_back(m_Scheduler->schedule([this](details::owner_dependency_pack pack)
								  {
									  return filter(pack);
								  }, pack).second);
		}
	}*/

	std::uintptr_t cache_offset = (std::uintptr_t)m_Cache.data();
	for(auto& system : m_SystemInformations)
	{
		PROFILE_SCOPE(core::profiler)

		std::vector<command_buffer> cmds{prepare_system(dTime, dTime, m_Cache, cache_offset, system)};

		// for(auto i = 0; i < cmds.size(); ++i) execute_command_buffer(cmds[i]);
	}

	for(auto&[key, entities] : m_StateChange[(m_Tick+1) % 2].removed_components)
	{
		if(entities.size() == 0) continue;
		std::vector<entity> erased_entities;
		auto compIt = m_Components.find(key);
		if(compIt == std::end(m_Components)) continue;
		std::sort(std::begin(entities), std::end(entities));
		std::set_intersection(std::begin(entities), std::end(entities), std::begin(compIt->second.entities),
							  std::end(compIt->second.entities), std::back_inserter(erased_entities));
		entities = erased_entities;
		std::vector<entity> remaining_entities;
		std::set_difference(std::begin(compIt->second.entities), std::end(compIt->second.entities),
							std::begin(erased_entities), std::end(erased_entities),
							std::back_inserter(remaining_entities));
		compIt->second.entities = remaining_entities;
	}
	destroy_immediate(m_StateChange[m_Tick % 2].removed_entities);
	for(const auto&[key, entities] : m_StateChange[m_Tick % 2].removed_components)
		destroy_immediate(key, entities);
	m_StateChange[m_Tick % 2].clear();
}

void state::set(psl::array_view<entity> entities, void* data, size_t size, component_key_t id) const noexcept
{
	if(entities.size() == 0) return;
	const auto& mem_pair = m_Components.find(id);
	if(mem_pair->second.is_tag()) return;
	std::uintptr_t data_loc = (std::uintptr_t)data;
	for(const auto& [i, e] : psl::enumerate(entities))
	{
		auto eMapIt  = m_EntityMap.find(e);
		auto foundIt = std::find_if(eMapIt->second.begin(), eMapIt->second.end(),
									[&id](const std::pair<component_key_t, size_t>& pair) { return pair.first == id; });

		auto index = foundIt->second;
		void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + size * index);
		std::memcpy(loc, (void*)data_loc, size);
		data_loc += size;
	}
}

std::vector<entity> state::dynamic_filter(psl::array_view<component_key_t> keys,
										  std::optional<psl::array_view<entity>> pre_selection) const noexcept
{
	PROFILE_SCOPE(core::profiler)

	for(const auto& key : keys)
	{
		if(m_Components.find(key) == std::end(m_Components)) return {};
	}

	const auto& first_selection = m_Components.at(keys[0]);

	std::vector<entity> v_intersection{};
	if(pre_selection)
	{
		v_intersection.reserve(std::min(pre_selection.value().size(), first_selection.entities.size()));

		std::set_intersection(std::begin(first_selection.entities), std::end(first_selection.entities),
							  std::begin(pre_selection.value()), std::end(pre_selection.value()),
							  std::back_inserter(v_intersection));
	}
	else
		v_intersection = first_selection.entities;

	for(size_t i = 1; i < keys.size(); ++i)
	{
		std::vector<entity> intermediate;
		intermediate.reserve(v_intersection.size());
		const auto& it = m_Components.at(keys[i]).entities;
		std::set_intersection(v_intersection.begin(), v_intersection.end(), it.begin(), it.end(),
							  std::back_inserter(intermediate));
		v_intersection = std::move(intermediate);
		if(v_intersection.size() == 0) return v_intersection;
	}

	return v_intersection;
}

std::vector<entity>
state::dynamic_filter(psl::array_view<component_key_t> keys,
					  const details::key_value_container_t<component_key_t, std::vector<entity>>& container,
					  std::optional<psl::array_view<entity>> pre_selection) const noexcept
{
	PROFILE_SCOPE(core::profiler)

	for(const auto& key : keys)
	{
		if(container.find(key) == std::end(container)) return {};
	}


	const auto& first_selection = container.at(keys[0]);

	std::vector<entity> v_intersection{};
	if(pre_selection)
	{
		v_intersection.reserve(std::min(pre_selection.value().size(), first_selection.size()));

		std::set_intersection(std::begin(first_selection), std::end(first_selection), std::begin(pre_selection.value()),
							  std::end(pre_selection.value()), std::back_inserter(v_intersection));
	}
	else
		v_intersection = first_selection;

	for(size_t i = 1; i < keys.size(); ++i)
	{
		std::vector<entity> intermediate;
		intermediate.reserve(v_intersection.size());
		const auto& it = container.at(keys[i]);
		std::set_intersection(v_intersection.begin(), v_intersection.end(), it.begin(), it.end(),
							  std::back_inserter(intermediate));
		v_intersection = std::move(intermediate);
	}

	return v_intersection;
}


void state::fill_in(psl::array_view<entity> entities, component_key_t int_id, void* out) const noexcept
{
	PROFILE_SCOPE(core::profiler)

	const auto& mem_pair	  = m_Components.find(int_id);
	const auto size			  = mem_pair->second.size;
	const std::uintptr_t data = (std::uintptr_t)mem_pair->second.region.data();
	for(const auto& [i, e] : psl::enumerate(entities))
	{
		auto eMapIt = m_EntityMap.find(e);
		auto foundIt =
			std::find_if(eMapIt->second.begin(), eMapIt->second.end(),
						 [&int_id](const std::pair<component_key_t, size_t>& pair) { return pair.first == int_id; });

		auto index = foundIt->second;
		void* loc  = (void*)(data + size * index);
		std::memcpy((void*)((std::uintptr_t)out + (i * size)), loc, size);
	}
}


void state::destroy_component_generator_ids(details::component_info& cInfo, psl::array_view<entity> entities)
{
	if(entities.size() == 0) return;
	std::vector<size_t> IDs;
	for(auto e : entities)
	{
		if(auto it = m_EntityMap.find(e); it != std::end(m_EntityMap))
		{
			auto pair = std::find_if(
				std::begin(it->second), std::end(it->second),
				[&cInfo](const std::pair<component_key_t, size_t>& pair) { return pair.first == cInfo.id; });
			if(pair == std::end(it->second)) continue;
			IDs.emplace_back(pair->second);
			it->second.erase(pair);
		}
	}
	if(IDs.size() == 0 || cInfo.is_tag()) return;
	std::sort(std::begin(IDs), std::end(IDs));
	size_t cachedStartIndex = IDs[0];
	size_t cachedPrevIndex  = IDs[0];
	for(auto i = 1; i < IDs.size(); ++i)
	{
		++cachedPrevIndex;
		if(IDs[i] != cachedPrevIndex)
		{
			cInfo.generator.destroy(cachedStartIndex, cachedPrevIndex - cachedStartIndex);

			cachedPrevIndex  = IDs[i];
			cachedStartIndex = IDs[i];
		}
	}
	if(cachedStartIndex == cachedPrevIndex) cachedPrevIndex += 1;
	cInfo.generator.destroy(cachedStartIndex, cachedPrevIndex - cachedStartIndex);
}

void state::execute_command_buffer(command_buffer& cmds)
{
	// we shift the current ID with the difference of the highest ID generated in the command
	const auto id_difference_n = mID - cmds.m_StartID;
	cmds.apply(id_difference_n);
	mID += cmds.mID - cmds.m_StartID;

	// add entities
	if(cmds.m_NewEntities.size() > 0)
	{
		for(auto e : cmds.m_NewEntities)
		{
			m_EntityMap.emplace(e, std::vector<std::pair<component_key_t, size_t>>{});
		}
		m_StateChange[(m_Tick + 1) % 2].added_entities.insert(std::end(m_StateChange[(m_Tick + 1) % 2].added_entities),
															  std::begin(cmds.m_NewEntities),
															  std::end(cmds.m_NewEntities));
	}
	// add components
	const auto& entityMap = cmds.m_EntityMap;
	for(const auto& [key, cInfo] : cmds.m_Components)
	{
		// https://stackoverflow.com/questions/46114214/lambda-implicit-capture-fails-with-variable-declared-from-structured-binding
		// workaround for wording in standard that is being followed by clang/gcc implicitly.
		const auto& named_key{key};
		const auto& named_cInfo{cInfo};
		copy_components(key, cInfo.entities, cInfo.size, [&named_key, &named_cInfo, &entityMap](entity e) {
			auto eIt = entityMap.find(e);
			auto cIt = std::find_if(
				std::begin(eIt->second), std::end(eIt->second),
				[&named_key](const std::pair<component_key_t, size_t> keyPair) { return named_key == keyPair.first; });
			return ((std::uintptr_t)named_cInfo.region.data() + (named_cInfo.size * cIt->second));
		});
		m_StateChange[(m_Tick + 1) % 2].added_components[key].insert(
			std::end(m_StateChange[(m_Tick + 1) % 2].added_components[key]), std::begin(cInfo.entities),
			std::end(cInfo.entities));
	}

	// remove entities
	destroy(cmds.m_MarkedForDestruction);

	// remove components
	for(const auto& [key, entities] : cmds.m_ErasedComponents)
	{
		m_StateChange[(m_Tick + 1) % 2].removed_components[key].insert(
			std::end(m_StateChange[(m_Tick + 1) % 2].removed_components[key]), std::begin(entities),
			std::end(entities));
	}
}