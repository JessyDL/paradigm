#include "ecs/command_buffer.h"
#include "ecs/state.h"
#include "assertions.h"

using namespace psl::ecs;

command_buffer::command_buffer(const state& state) : m_State(&state), m_First(state.capacity()) {};


details::command_buffer_component_info* command_buffer::get_command_buffer_component_info(details::component_key_t key) noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });

	return (it != std::end(m_Components)) ? it->operator->() : nullptr;
}


// empty construction
void command_buffer::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
							   size_t size)
{
	auto cInfo = get_command_buffer_component_info(key);

	cInfo->add(entities);
}


// invocable based construction
void command_buffer::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
							   size_t size, std::function<void(std::uintptr_t, size_t)> invocable)
{
	assert(size != 0);
	auto cInfo = get_command_buffer_component_info(key);
	assert(cInfo != nullptr);

	auto offset = cInfo->count();
	cInfo->add(entities);

	auto location = (std::uintptr_t)cInfo->data() + (offset * size);
	std::invoke(invocable, location, entities.size());
}

// prototype based construction
void command_buffer::add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
							   size_t size, void* prototype)
{
	assert(size != 0);
	auto cInfo = get_command_buffer_component_info(key);
	assert(cInfo != nullptr);

	auto offset = cInfo->count();
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
void command_buffer::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size)
{
	auto cInfo = get_command_buffer_component_info(key);
	assert(cInfo != nullptr);

	cInfo->add(entities);
}

// invocable based construction
void command_buffer::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
							   std::function<void(std::uintptr_t, size_t)> invocable)
{
	assert(size != 0);
	auto cInfo = get_command_buffer_component_info(key);
	assert(cInfo != nullptr);

	auto offset = cInfo->count();
	cInfo->add(entities);

	auto location = (std::uintptr_t)cInfo->data() + (offset * size);
	std::invoke(invocable, location, entities.size());
}

// prototype based construction
void command_buffer::add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
							   void* prototype)
{
	assert(size != 0);
	auto cInfo = get_command_buffer_component_info(key);
	assert(cInfo != nullptr);

	auto offset = cInfo->count();

	cInfo->add(entities);
	for(auto e : entities)
	{
		std::memcpy((void*)((std::uintptr_t)cInfo->data() + (offset++) * size), prototype, size);
	}
}


void command_buffer::remove_component(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities) noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });
	assert(it != std::end(m_Components));
	(*it)->destroy(entities);
}


void command_buffer::remove_component(details::component_key_t key, psl::array_view<entity> entities) noexcept
{
	auto it = std::find_if(std::begin(m_Components), std::end(m_Components),
						   [key](const auto& cInfo) { return cInfo->id() == key; });
	assert(it != std::end(m_Components));


	(*it)->destroy(entities);
}

// consider an alias feature
// ie: alias transform = position, rotation, scale components
void command_buffer::destroy(psl::array_view<entity> entities) noexcept
{
	/*for(auto& cInfo : m_Components)
	{
		cInfo->destroy(entities);
	}*/
	//m_DestroyedEntities.insert(std::end(m_DestroyedEntities), std::begin(entities), std::end(entities));

	for(auto e : entities)
	{
		if(e < m_First)
		{
			m_DestroyedEntities.emplace_back(e);
			continue;
		}
		++m_Orphans;
		m_Entities[e] = m_Next;
		m_Next = e;
	}
}

void command_buffer::destroy(entity entity) noexcept
{
	/*for(auto& cInfo : m_Components)
	{
		cInfo->destroy(entity);
	}*/

	m_DestroyedEntities.emplace_back(entity);
	if(entity < m_First)
		return;

	m_Entities[entity] = m_Next;
	m_Next = entity;

	++m_Orphans;
}
