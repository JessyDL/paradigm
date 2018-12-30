#include "stdafx.h"
#include "ecs/state.h"

using namespace core::ecs;

void state::destroy(psl::array_view<entity> entities) noexcept
{
	PROFILE_SCOPE(core::profiler)

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
}

void state::destroy(entity e) noexcept { destroy(psl::array_view<entity>{&e, &e + 1}); }

void state::tick(std::chrono::duration<float> dTime)
{
	PROFILE_SCOPE(core::profiler)
	for(auto& system : m_Systems)
	{
		core::profiler.scope_begin("ticking system");
		auto& sBindings				= system.second.tick_dependencies;
		std::uintptr_t cache_offset = (std::uintptr_t)m_Cache.data();
		core::profiler.scope_begin("preparing data");
		for(auto& dep_pack : sBindings)
		{
			auto entities = dynamic_filter(dep_pack.filters);
			std::memcpy((void*)cache_offset, entities.data(), sizeof(entity) * entities.size());
			dep_pack.m_StoredEnts =  psl::array_view<core::ecs::entity>((entity*)cache_offset, (entity*)(cache_offset + sizeof(entity) * entities.size()));
			if(dep_pack.m_Entities != nullptr)
				*dep_pack.m_Entities = dep_pack.m_StoredEnts;
			cache_offset += sizeof(entity) * entities.size();
			core::profiler.scope_begin("read-write data");
			for(const auto& rwBinding : dep_pack.m_RWBindings)
			{
				const auto& mem_pair = m_Components.find(rwBinding.first);				
				const auto size			 = dep_pack.m_Sizes[rwBinding.first];
				const auto id				 = rwBinding.first;
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

				*rwBinding.second = psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache_offset);

				size_t x = 0;
			}
			core::profiler.scope_end();
			core::profiler.scope_begin("read-only data");
			for(const auto& rBinding : dep_pack.m_RBindings)
			{
				const auto& mem_pair = m_Components.find(rBinding.first);
				const auto size			 = dep_pack.m_Sizes[rBinding.first];
				const auto id				 = rBinding.first;

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

				*rBinding.second = psl::array_view<std::uintptr_t>((std::uintptr_t*)data_begin, (std::uintptr_t*)cache_offset);
			}
			core::profiler.scope_end();
		}
		core::profiler.scope_end();
		std::invoke(system.second.tick, *this, dTime * 0.01f, dTime);
		for(const auto& dep_pack : sBindings)
		{
			for(const auto& rwBinding : dep_pack.m_RWBindings)
			{
				const size_t size = dep_pack.m_Sizes.at(rwBinding.first);
				std::uintptr_t data = (std::uintptr_t)&std::begin(*rwBinding.second).value();
				set(dep_pack.m_StoredEnts, (void*)data, size, rwBinding.first);
			}
		}
		core::profiler.scope_end();
	}


	m_AddedEntities.clear();
	m_RemovedEntities.clear();
	m_AddedComponents.clear();
	m_RemovedComponents.clear();
}

void state::set(psl::array_view<entity> entities, void* data, size_t size, details::component_key_t id)
{
	PROFILE_SCOPE(core::profiler)
	const auto& mem_pair = m_Components.find(id);

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

std::vector<entity> state::dynamic_filter(psl::array_view<details::component_key_t> keys) const noexcept
{
	PROFILE_SCOPE(core::profiler)

	for(const auto& key : keys)
	{
		if(m_Components.find(key) == std::end(m_Components)) return {};
	}

	std::vector<entity> v_intersection{m_Components.at(keys[0]).entities};

	for(size_t i = 1; i < keys.size(); ++i)
	{
		std::vector<entity> intermediate;
		intermediate.reserve(v_intersection.size());
		const auto& it = m_Components.at(keys[i]).entities;
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
