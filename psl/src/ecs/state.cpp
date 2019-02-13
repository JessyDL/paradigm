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
	++m_ChangeSetTick;
	// tick systems;

	// purge m_Changes[m_Tick];

	//for(const auto&[key, entities] : m_Changes[m_Tick % 2].removed_components) m_Components[key].purge();

	m_Changes[m_Tick % 2] = {};
	++m_Tick;
}

details::component_info& state::get_component_info(component_key_t key, size_t size)
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components), [key](const auto& pair)
						   { return pair.first == key; });

	if(it != std::end(m_Components)) return it->second;

	return m_Components.emplace_back(key, details::component_info{key, size, 3000000}).second;
}

psl::array<entity> prepare_for_add_component(component_key_t key, psl::array_view<entity> entities_view,
											 const psl::bytell_map<entity, entity_info>& map)
{
	std::vector<entity> entities{entities_view};
#if defined(_DEBUG) && defined(SAFE_ECS)
	auto size = std::distance(std::begin(entities),
							  std::remove_if(std::begin(entities), std::end(entities), [&map, key](const entity& e) {
								  auto eMapIt = map.find(e);
								  if(eMapIt == std::end(map))
								  {
									  return true;
								  }

								  return eMapIt->second.has(key);
							  }));
	assert(entities.size() == size);
#endif
	return entities;
}

// empty construction
void state::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities, size_t size)
{
	auto& cInfo						   = get_component_info(key, size);

	auto ids = cInfo.add(entities);

	if(!cInfo.is_tag())
	{
		for(const auto& id_range : ids)
		{
			auto location = (std::uintptr_t)cInfo.data() + (id_range.first * size);
			std::memset((void*)(location + id_range.first * size), 0, size * (id_range.second - id_range.first));
		}
	}

	auto& added_components = m_Changes[m_ChangeSetTick % 2].added_components[key];
	added_components.insert(std::end(added_components), std::begin(entities), std::end(entities));
}

// invocable based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities, size_t size,
							   std::function<void(std::uintptr_t, size_t)> invocable)
{
	assert(size != 0);
	auto& cInfo						   = get_component_info(key, size);

	auto ids = cInfo.add(entities);
	for(const auto& id_range : ids)
	{
		auto location = (std::uintptr_t)cInfo.data() + (id_range.first * size);
		std::invoke(invocable, location, id_range.second - id_range.first);
	}

	auto& added_components = m_Changes[m_ChangeSetTick % 2].added_components[key];
	added_components.insert(std::end(added_components), std::begin(entities), std::end(entities));
}

// prototype based construction
void state::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities, size_t size,
							   void* prototype)
{
	assert(size != 0);
	auto& cInfo						   = get_component_info(key, size);

	auto ids = cInfo.add(entities);
	for(const auto& id_range : ids)
	{
		for(auto i = id_range.first; i < id_range.second; ++i)
		{
			std::memcpy((void*)((std::uintptr_t)cInfo.data() + i * size), prototype, size);
		}
	}

	auto& added_components = m_Changes[m_ChangeSetTick % 2].added_components[key];
	added_components.insert(std::end(added_components), std::begin(entities), std::end(entities));
}


psl::array<entity> state::filter_impl(details::component_key_t key,
									  std::optional<psl::array_view<entity>> entities) const noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components), [key](const auto& pair)
						   {
							   return pair.first == key;
						   });
	if(it == std::end(m_Components)) return {};

	if(entities)
	{
		psl::array<entity> result;
		std::set_intersection(std::begin(entities.value()), std::end(entities.value()),
							  std::begin(it->second.entities()), std::end(it->second.entities()),
							  std::back_inserter(result));
		return result;
	}

	return psl::array<entity>{it->second.entities()};
}

psl::array<entity> state::filter_except_impl(details::component_key_t key,
											 std::optional<psl::array_view<entity>> entities) const noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components), [key](const auto& pair)
						   {
							   return pair.first == key;
						   });
	if(it == std::end(m_Components)) return {};

	if(entities)
	{
		psl::array<entity> result;
		std::set_difference(std::begin(entities.value()), std::end(entities.value()), std::begin(it->second.entities()),
							std::end(it->second.entities()), std::back_inserter(result));
		return result;
	}

	return psl::array<entity>{it->second.entities()};
}

psl::array<entity> state::filter_impl(details::component_key_t key,
									  const psl::bytell_map<details::component_key_t, psl::array<entity>>& map,
									  std::optional<psl::array_view<entity>> entities) const noexcept
{
	auto it = map.find(key);
	if(it == std::end(map)) return {};

	if(entities)
	{
		psl::array<entity> result;
		std::set_intersection(std::begin(entities.value()), std::end(entities.value()), std::begin(it->second),
							  std::end(it->second), std::back_inserter(result));
		return result;
	}

	return it->second;
}

void state::remove_component(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities) noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components), [key](const auto& pair)
						   {
							   return pair.first == key;
						   });
	assert(it != std::end(m_Components));
	if(it == std::end(m_Components)) return;

	psl::array<uint64_t> indices;
	auto& destroyed_data = m_Changes[m_ChangeSetTick % 2].destroyed_data;


	it->second.destroy(entities);

	auto& removed_components = m_Changes[m_ChangeSetTick % 2].removed_components[key];
	removed_components.insert(std::end(removed_components), std::begin(entities), std::end(entities));
}

void state::destroy(psl::array_view<std::pair<entity, entity>> entities) noexcept
{
	for(auto& [key, cInfo] : m_Components)
	{
		cInfo.destroy(entities);
	}

	for(auto range : entities)
	{
		m_Generator.destroy(range.first, range.second - range.first);
	}
}


void state::destroy(entity entity) noexcept
{
	for(auto&[key, cInfo] : m_Components)
	{
		cInfo.destroy(entity);
	}
	m_Generator.destroy(entity);
}