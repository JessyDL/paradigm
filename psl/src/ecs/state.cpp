#include "ecs/state.h"
#include "task.h"

using namespace psl::ecs;

using psl::ecs::details::component_key_t;
using psl::ecs::details::entity_info;


state::state(size_t workers)
	: m_Scheduler(new psl::async::scheduler((workers == 0) ? std::nullopt : std::optional{workers}))
{}


void state::tick(std::chrono::duration<float> dTime)
{
	// tick systems;

	// purge m_Changes[m_Tick];


	++m_Tick;
}

details::component_info& state::get_component_info(component_key_t key, size_t size)
{
	auto it = m_Components.find(key);

	if(it != std::end(m_Components)) return it->second;

	return m_Components.emplace(key, details::component_info{key, size}).first->second;
}

psl::array<entity> prepare_for_add_component(component_key_t key, psl::array_view<entity> entities_view,
											 const psl::bytell_map<entity, entity_info>& map)
{
	std::vector<entity> entities = entities_view;
#if defined(_DEBUG)
	auto size = std::distance(std::begin(entities),
							  std::remove_if(std::begin(entities), std::end(entities), [&map, key](const entity& e) {
								  auto eMapIt = map.find(e);
								  if(eMapIt == std::end(map))
								  {
									  return true;
								  }

								  for(auto eComp : eMapIt->second.components)
								  {
									  if(eComp == key) return true;
								  }
								  return false;
							  }));
	assert(entities.size() == size);
#endif
	std::sort(std::begin(entities), std::end(entities));
	return entities;
}

// empty construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size)
{
	auto& cInfo						   = get_component_info(key, size);
	psl::array<entity> unique_entities = prepare_for_add_component(key, entities, m_Entities);
	entities						   = unique_entities;

	auto ids = cInfo.add(entities);
	m_Entities.reserve(m_Entities.size() + entities.size());
	if(cInfo.is_tag())
	{
		for(auto eIt = std::begin(entities); eIt != std::end(entities); ++eIt)
		{
			const entity& e{*eIt};
			m_Entities[e].emplace_back(key, 0);
		}
	}
	else
	{
		auto eIt = std::begin(entities);
		for(const auto& id_range : ids)
		{
			for(auto i = id_range.first; i < id_range.first + id_range.second; ++i)
			{
				const entity& e{*eIt};
				m_Entities[e].emplace_back(key, i);
				std::memset((void*)((std::uintptr_t)cInfo.data() + i * size), 0, size);
				eIt = std::next(eIt);
			}
		}
	}
}

// invocable based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
							   std::function<void(std::uintptr_t, size_t)> invocable)
{
	assert(size != 0);
	auto& cInfo						   = get_component_info(key, size);
	psl::array<entity> unique_entities = prepare_for_add_component(key, entities, m_Entities);
	entities						   = unique_entities;

	auto ids = cInfo.add(entities);
	m_Entities.reserve(m_Entities.size() + entities.size());

	auto eIt = std::begin(entities);
	for(const auto& id_range : ids)
	{
		for(auto i = id_range.first; i < id_range.first + id_range.second; ++i)
		{
			const entity& e{*eIt};
			m_Entities[e].emplace_back(key, i);
			eIt = std::next(eIt);
		}

		auto location = (std::uintptr_t)cInfo.data() + (id_range.first * size);
		std::invoke(invocable, location, id_range.second);
	}
}

// prototype based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
							   void* prototype)
{
	assert(size != 0);
	auto& cInfo						   = get_component_info(key, size);
	psl::array<entity> unique_entities = prepare_for_add_component(key, entities, m_Entities);
	entities						   = unique_entities;

	auto ids = cInfo.add(entities);
	m_Entities.reserve(m_Entities.size() + entities.size());

	auto eIt = std::begin(entities);
	for(const auto& id_range : ids)
	{
		for(auto i = id_range.first; i < id_range.first + id_range.second; ++i)
		{
			const entity& e{*eIt};
			m_Entities[e].emplace_back(key, i);
			std::memcpy((void*)((std::uintptr_t)cInfo.data() + i * size), prototype, size);
			eIt = std::next(eIt);
		}
	}
}


void state::remove_component(details::component_key_t key, psl::array_view<entity> entities) noexcept {}