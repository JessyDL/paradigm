#include "ecs/command_buffer.h"
#include "ecs/state.h"
#include <functional>

using namespace core::ecs;


command_buffer::command_buffer(const state& state, uint64_t id_offset) : m_State(state), mID(id_offset), m_StartID(id_offset) {}

command_buffer::command_buffer(const state& state)  : m_State(state), mID(m_State.mID), m_StartID(mID) {}

void command_buffer::verify_entities(psl::array_view<entity> entities)
{
	for(auto entity : entities)
	{
		if(entity <= m_StartID && m_State.exists(entity))
			m_EntityMap.emplace(entity, std::vector<std::pair<component_key_t, size_t>>{});
	}
}

void command_buffer::apply(size_t id_difference_n)
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

	for(auto&[key, cInfo] : m_Components)
	{
		std::vector<ecs::entity> actual_entities;
		std::set_difference(std::begin(cInfo.entities), std::end(cInfo.entities), std::begin(removed_entities),
							std::end(removed_entities), std::back_inserter(actual_entities));
		cInfo.entities = actual_entities;
		for(auto& e : cInfo.entities)
		{
			if(e.id() > m_StartID)
				e = entity{e.id() + id_difference_n};
		}
	}

	for(auto&[key, entities] : m_ErasedComponents)
	{
		std::vector<entity> removed_components;
		std::set_difference(std::begin(entities), std::end(entities), std::begin(removed_entities),
							std::end(removed_entities), std::back_inserter(removed_components));

		entities = removed_components;
		for(auto& e : entities)
		{
			if(e.id() > m_StartID)
				e = entity{e.id() + id_difference_n};
		}
	}

	m_NewEntities = added_entities;
	m_MarkedForDestruction = destroyed_entities;
	for(auto& e : added_entities)
	{
		if(e.id() > m_StartID)
			e = entity{e.id() + id_difference_n};
	}
	for(auto& e : destroyed_entities)
	{
		if(e.id() > m_StartID)
			e = entity{e.id() + id_difference_n};
	}

	details::key_value_container_t<entity, std::vector<std::pair<component_key_t, size_t>>> old_entity_map{std::move(m_EntityMap)};
	m_EntityMap = details::key_value_container_t<entity, std::vector<std::pair<component_key_t, size_t>>>{};
	for(auto&[e, vec] : old_entity_map)
	{
		entity ent{e};
		if(e.id() > m_StartID)
			ent = entity{e.id() + id_difference_n};

		m_EntityMap.emplace(ent, std::move(vec));
	}
}