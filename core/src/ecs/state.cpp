#include "ecs/state.h"
#include <execution>
#include "enumerate.h"

using namespace core::ecs;

void state::destroy(psl::array_view<entity> entities) noexcept
{
	PROFILE_SCOPE(core::profiler)
	if (entities.size() == 0)
		return;

	ska::bytell_hash_map<details::component_key_t, std::vector<entity>> erased_entities;
	ska::bytell_hash_map<details::component_key_t, std::vector<uint64_t>> erased_ids;
	core::profiler.scope_begin("erase entities");
	for(const auto& e : entities)
	{
		if(auto eMapIt = m_EntityMap.find(e); eMapIt != std::end(m_EntityMap))
		{
			for(const auto& [type, index] : eMapIt->second)
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
			if(c.second.size() > 64)
			{
				std::sort(std::begin(c.second), std::end(c.second));
				auto index		 = std::begin(c.second);
				auto range_start = index;
				const auto end   = std::prev(std::end(c.second), 1);
				while(index != end)
				{
					auto next = std::next(index, 1);
					if(*index + 1 != *next)
					{
						cMapIt->second.generator.DestroyRangeID(*range_start, std::distance(range_start, next));
						range_start = next;
					}
					index = next;
				}
				cMapIt->second.generator.DestroyRangeID(*range_start, std::distance(range_start, std::end(c.second)));
			}
			else
			{
				for(auto id : c.second) cMapIt->second.generator.DestroyID(id);
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
			auto ib   = std::begin(c.second);
			auto iter = std::remove_if(std::begin(cMapIt->second.entities), std::end(cMapIt->second.entities),
									   [&ib, &c](entity x) -> bool {
										   while(ib != std::end(c.second) && *ib < x) ++ib;
										   return (ib != std::end(c.second) && *ib == x);
									   });

			cMapIt->second.entities.erase(iter, cMapIt->second.entities.end());
		}
	}
	core::profiler.scope_end();
	m_StateChange[(m_Tick + 1) % 2].removed_entities.insert(std::end(m_StateChange[(m_Tick + 1) % 2].removed_entities), std::begin(entities), std::end(entities));
}

void state::destroy(entity e) noexcept { destroy(psl::array_view<entity>{&e, &e + 1}); }

void state::prepare_system(std::chrono::duration<float> dTime, memory::raw_region& cache, size_t cache_offset, void* system, std::vector<dependency_pack>& bindings)
{
	PROFILE_SCOPE(core::profiler);
	for(auto& dep_pack : bindings)
	{
		auto entities = filter(dep_pack);
		std::memcpy((void*)cache_offset, entities.data(), sizeof(entity) * entities.size());
		dep_pack.m_StoredEnts = psl::array_view<core::ecs::entity>(
			(entity*)cache_offset, (entity*)(cache_offset + sizeof(entity) * entities.size()));
		if(dep_pack.m_Entities != nullptr) *dep_pack.m_Entities = dep_pack.m_StoredEnts;
		cache_offset += sizeof(entity) * entities.size();

		//std::for_each(std::execution::par_unseq, )
	}
}

void state::tick(std::chrono::duration<float> dTime)
{
	PROFILE_SCOPE(core::profiler)
	++m_Tick;
	for(auto& system : m_Systems)
	{
		core::profiler.scope_begin("ticking system");
		auto& sBindings				= system.second.tick_dependencies;
		std::uintptr_t cache_offset = (std::uintptr_t)m_Cache.data();
		core::profiler.scope_begin("preparing data");
		for(auto& dep_pack : sBindings)
		{
			auto entities = filter(dep_pack);
			std::memcpy((void*)cache_offset, entities.data(), sizeof(entity) * entities.size());
			dep_pack.m_StoredEnts = psl::array_view<core::ecs::entity>(
				(entity*)cache_offset, (entity*)(cache_offset + sizeof(entity) * entities.size()));
			if(dep_pack.m_Entities != nullptr) *dep_pack.m_Entities = dep_pack.m_StoredEnts;
			cache_offset += sizeof(entity) * entities.size();
			core::profiler.scope_begin("read-write data");
			for(const auto& rwBinding : dep_pack.m_RWBindings)
			{
				const auto& mem_pair	  = m_Components.find(rwBinding.first);
				const auto size			  = dep_pack.m_Sizes[rwBinding.first];
				const auto id			  = rwBinding.first;
				std::uintptr_t data_begin = cache_offset;

				for(const auto& e : entities)
				{

					auto eMapIt  = m_EntityMap.find(e);
					auto foundIt = std::find_if(
						eMapIt->second.begin(), eMapIt->second.end(),
						[&id](const std::pair<details::component_key_t, size_t>& pair) { return pair.first == id; });

					auto index = foundIt->second;
					void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + size * index);
					std::memcpy((void*)cache_offset, loc, size);
					cache_offset += size;
				}

				*rwBinding.second =
					psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache_offset);

				size_t x = 0;
			}
			core::profiler.scope_end();
			core::profiler.scope_begin("read-only data");
			for(const auto& rBinding : dep_pack.m_RBindings)
			{
				const auto& mem_pair = m_Components.find(rBinding.first);
				const auto size		 = dep_pack.m_Sizes[rBinding.first];
				const auto id		 = rBinding.first;

				std::uintptr_t data_begin = cache_offset;

				for(const auto& e : entities)
				{

					auto eMapIt  = m_EntityMap.find(e);
					auto foundIt = std::find_if(
						eMapIt->second.begin(), eMapIt->second.end(),
						[&id](const std::pair<details::component_key_t, size_t>& pair) { return pair.first == id; });

					auto index = foundIt->second;
					void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + size * index);
					std::memcpy((void*)cache_offset, loc, size);
					cache_offset += size;
				}

				*rBinding.second =
					psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache_offset);
			}
			core::profiler.scope_end();
		}
		core::profiler.scope_end();
		commands cmds{*this, mID};

		if(system.second.tick)
			std::invoke(system.second.tick, cmds, dTime, dTime);

		for(const auto& dep_pack : sBindings)
		{
			for(const auto& rwBinding : dep_pack.m_RWBindings)
			{
				const size_t size   = dep_pack.m_Sizes.at(rwBinding.first);
				std::uintptr_t data = (std::uintptr_t)&std::begin(*rwBinding.second).value();
				set(dep_pack.m_StoredEnts, (void*)data, size, rwBinding.first);
			}
		}
		execute_commands(cmds);
		core::profiler.scope_end();
	}

	m_StateChange[m_Tick % 2].clear();
}

void state::set(psl::array_view<entity> entities, void* data, size_t size, details::component_key_t id)
{
	PROFILE_SCOPE(core::profiler)
	if(entities.size() == 0)
		return;
	const auto& mem_pair = m_Components.find(id);
	if(mem_pair->second.size == 1)
		return;
	std::uintptr_t data_loc = (std::uintptr_t)data;
	for(const auto& [i, e] : psl::enumerate(entities))
	{
		auto eMapIt = m_EntityMap.find(e);
		auto foundIt =
			std::find_if(eMapIt->second.begin(), eMapIt->second.end(),
						 [&id](const std::pair<details::component_key_t, size_t>& pair) { return pair.first == id; });

		auto index = foundIt->second;
		void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + size * index);
		std::memcpy(loc, (void*)data_loc, size);
		data_loc += size;
	}
}

std::vector<entity> state::dynamic_filter(psl::array_view<details::component_key_t> keys,
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
state::dynamic_filter(psl::array_view<details::component_key_t> keys,
					  const details::key_value_container_t<details::component_key_t, std::vector<entity>>& container,
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


void state::fill_in(psl::array_view<entity> entities, details::component_key_t int_id, void* out) const noexcept
{
	PROFILE_SCOPE(core::profiler)

	const auto& mem_pair	  = m_Components.find(int_id);
	const auto size			  = mem_pair->second.size;
	const std::uintptr_t data = (std::uintptr_t)mem_pair->second.region.data();
	for(const auto& [i, e] : psl::enumerate(entities))
	{
		auto eMapIt  = m_EntityMap.find(e);
		auto foundIt = std::find_if(
			eMapIt->second.begin(), eMapIt->second.end(),
			[&int_id](const std::pair<details::component_key_t, size_t>& pair) { return pair.first == int_id; });

		auto index = foundIt->second;
		void* loc  = (void*)(data + size * index);
		std::memcpy((void*)((std::uintptr_t)out + (i * size)), loc, size);
	}
}

commands::commands(state& state, uint64_t id_offset) : m_State(state), m_StartID(id_offset), mID(id_offset) {}

void commands::apply(size_t id_difference_n)
{
	std::vector<entity> added_entities;
	std::vector<entity> removed_entities;
	std::vector<entity> destroyed_entities;
	std::set_difference(std::begin(m_NewEntities), std::end(m_NewEntities), std::begin(m_MarkedForDestruction),
						std::end(m_MarkedForDestruction), std::back_inserter(added_entities));
	std::set_difference(std::begin(m_NewEntities), std::end(m_NewEntities), std::begin(added_entities),
						std::end(added_entities), std::back_inserter(removed_entities));
	std::set_difference(std::begin(m_MarkedForDestruction), std::end(m_MarkedForDestruction),
						std::begin(removed_entities), std::end(removed_entities),
						std::back_inserter(destroyed_entities));

	for(auto& [key, cInfo] : m_Components)
	{
		std::vector<ecs::entity> actual_entities;
		std::set_difference(std::begin(cInfo.entities), std::end(cInfo.entities), std::begin(removed_entities),
							std::end(removed_entities), std::back_inserter(actual_entities));
		cInfo.entities = actual_entities;
		for (auto& e : cInfo.entities)
		{
			if (e.id() > m_StartID)
				e = entity{ e.id() + id_difference_n };
		}
	}

	for (auto&[key, entities] : m_ErasedComponents)
	{
		std::vector<entity> removed_components;
		std::set_difference(std::begin(entities), std::end(entities), std::begin(removed_entities),
			std::end(removed_entities), std::back_inserter(removed_components));

		entities = removed_components;
		for (auto& e : entities)
		{
			if (e.id() > m_StartID)
				e = entity{ e.id() + id_difference_n };
		}
	}

	m_NewEntities		   = added_entities;
	m_MarkedForDestruction = destroyed_entities;
	for (auto& e : added_entities)
	{
		if (e.id() > m_StartID)
			e = entity{ e.id() + id_difference_n };
	}
	for (auto& e : destroyed_entities)
	{
		if (e.id() > m_StartID)
			e = entity{ e.id() + id_difference_n };
	}

	details::key_value_container_t<entity, std::vector<std::pair<details::component_key_t, size_t>>> old_entity_map{ std::move(m_EntityMap) };
	m_EntityMap = details::key_value_container_t<entity, std::vector<std::pair<details::component_key_t, size_t>>>{};
	for (auto&[e, vec] : old_entity_map)
	{
		entity ent{e};
		if (e.id() > m_StartID)
			ent = entity{ e.id() + id_difference_n };
		
		m_EntityMap.emplace(ent, std::move(vec));
	}
}


void state::destroy_component_generator_ids(details::component_info& cInfo, psl::array_view<entity> entities)
{
	if (entities.size() == 0)
		return;
	std::vector<size_t> IDs;
	for (auto e : entities)
	{
		if (auto it = m_EntityMap.find(e); it != std::end(m_EntityMap))
		{
			auto pair = std::remove_if(std::begin(it->second), std::end(it->second), [&cInfo](const std::pair<details::component_key_t, size_t>& pair)
			{ return pair.first == cInfo.id; });
			if (pair == std::end(it->second))
				continue;
			IDs.emplace_back(pair->second);
			it->second.erase(pair, std::end(it->second));
		}
	}
	if (IDs.size() == 0)
		return;
	std::sort(std::begin(IDs), std::end(IDs));
	size_t cachedStartIndex = IDs[0];
	size_t cachedPrevIndex = IDs[0];
	for (auto i = 1; i < IDs.size(); ++i)
	{
		++cachedPrevIndex;
		if (IDs[i] != cachedPrevIndex)
		{
			cInfo.generator.DestroyRangeID(cachedStartIndex, cachedPrevIndex - cachedStartIndex);

			cachedPrevIndex = IDs[i];
			cachedStartIndex = IDs[i];
		}
	}
	cInfo.generator.DestroyRangeID(cachedStartIndex, cachedPrevIndex - cachedStartIndex);
}

void state::execute_commands(commands& cmds)
{
	// we shift the current ID with the difference of the highest ID generated in the command
	const auto id_difference_n = mID - cmds.m_StartID;
	cmds.apply(id_difference_n);
	mID += cmds.mID - cmds.m_StartID;

	// add entities
	if (cmds.m_NewEntities.size() > 0)
	{
		for (auto e : cmds.m_NewEntities)
		{
			m_EntityMap.emplace(e, std::vector<std::pair<details::component_key_t, size_t>>{});
		}
		m_StateChange[(m_Tick + 1) % 2].added_entities.insert(std::end(m_StateChange[(m_Tick + 1) % 2].added_entities), std::begin(cmds.m_NewEntities), std::end(cmds.m_NewEntities));
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
				[&named_key](const std::pair<details::component_key_t, size_t> keyPair) { return named_key == keyPair.first; });
			return ((std::uintptr_t)named_cInfo.region.data() + (named_cInfo.size * cIt->second));
		});
		m_StateChange[(m_Tick + 1) % 2].added_components[key].insert(std::end(m_StateChange[(m_Tick + 1) % 2].added_components[key]), std::begin(cInfo.entities), std::end(cInfo.entities));
	}

	// remove entities
	destroy(cmds.m_MarkedForDestruction);

	// remove components
	for (const auto&[key, entities] : cmds.m_ErasedComponents)
	{
		if (entities.size() == 0)
			continue;
		std::vector<entity> erased_entities;
		auto compIt = m_Components.find(key);
		if (compIt == std::end(m_Components))
			continue;
		std::set_intersection(std::begin(entities), std::end(entities), std::begin(compIt->second.entities), std::end(compIt->second.entities), std::back_inserter(erased_entities));
		destroy_component_generator_ids(compIt->second, erased_entities);
		std::vector<entity> remaining_entities;
		std::set_difference(std::begin(compIt->second.entities), std::end(compIt->second.entities), std::begin(erased_entities), std::end(erased_entities), std::back_inserter(remaining_entities));
		compIt->second.entities = remaining_entities;
		m_StateChange[(m_Tick + 1) % 2].removed_components[key].insert(std::end(m_StateChange[(m_Tick + 1) % 2].removed_components[key]), std::begin(entities), std::end(entities));
	}
}