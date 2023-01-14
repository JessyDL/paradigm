#include "psl/ecs/command_buffer.hpp"
#include "psl/assertions.hpp"
#include "psl/ecs/state.hpp"

using namespace psl::ecs;

command_buffer_t::command_buffer_t(const state_t& state)
	: m_State(&state), m_First(static_cast<entity_t::size_type>(state.capacity())) {};


details::component_container_t*
command_buffer_t::get_component_container(const details::component_key_t& key) noexcept {
	auto it = std::find_if(
	  std::begin(m_Components), std::end(m_Components), [&key](const auto& cInfo) { return cInfo->id() == key; });

	return (it != std::end(m_Components)) ? it->operator->() : nullptr;
}


// empty construction
void command_buffer_t::add_component_impl(const details::component_key_t& key,
										  psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
										  size_t size) {
	auto cInfo = get_component_container(key);

	cInfo->add(entities);
}


// invocable based construction
void command_buffer_t::add_component_impl(const details::component_key_t& key,
										  psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
										  size_t size,
										  std::function<void(std::uintptr_t, size_t)> invocable) {
	psl_assert(size != 0, "size of requested components shouldn't be 0");
	auto cInfo = get_component_container(key);
	psl_assert(cInfo != nullptr, "component info for key {} was not found", key);

	auto offset = cInfo->size();
	cInfo->add(entities);

	auto location = (std::uintptr_t)cInfo->data() + (offset * size);
	std::invoke(invocable, location, entities.size());
}

// prototype based construction
void command_buffer_t::add_component_impl(const details::component_key_t& key,
										  psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities,
										  size_t size,
										  void* prototype) {
	psl_assert(size != 0, "size of requested components shouldn't be 0");
	auto cInfo = get_component_container(key);
	psl_assert(cInfo != nullptr, "component info for key {} was not found", key);

	auto offset = cInfo->size();
	cInfo->add(entities);
	for(const auto& id_range : entities) {
		for(auto i = static_cast<entity_t::size_type>(id_range.first); i < static_cast<entity_t::size_type>(id_range.second);
			++i) {
			std::memcpy((void*)((std::uintptr_t)cInfo->data() + (offset++) * size), prototype, size);
		}
	}
}


// empty construction
void command_buffer_t::add_component_impl(const details::component_key_t& key,
										  psl::array_view<entity_t> entities,
										  size_t size) {
	auto cInfo = get_component_container(key);
	psl_assert(cInfo != nullptr, "component info for key {} was not found", key);

	cInfo->add(entities);
}

// invocable based construction
void command_buffer_t::add_component_impl(const details::component_key_t& key,
										  psl::array_view<entity_t> entities,
										  size_t size,
										  std::function<void(std::uintptr_t, size_t)> invocable) {
	psl_assert(size != 0, "size of requested components shouldn't be 0");
	auto cInfo = get_component_container(key);
	psl_assert(cInfo != nullptr, "component info for key {} was not found", key);

	auto offset = cInfo->size();
	cInfo->add(entities);

	auto location = (std::uintptr_t)cInfo->data() + (offset * size);
	std::invoke(invocable, location, entities.size());
}

// prototype based construction
void command_buffer_t::add_component_impl(const details::component_key_t& key,
										  psl::array_view<entity_t> entities,
										  size_t size,
										  void* prototype,
										  bool repeat) {
	psl_assert(size != 0, "size of requested components shouldn't be 0");
	auto cInfo = get_component_container(key);
	psl_assert(cInfo != nullptr, "component info for key {} was not found", key);

	auto offset = cInfo->size();

	cInfo->add(entities);
	if(repeat) {
		for(auto e : entities) {
			std::memcpy((void*)((std::uintptr_t)cInfo->data() + (offset++) * size), prototype, size);
		}
	} else {
		for(auto e : entities) {
			std::memcpy((void*)((std::uintptr_t)cInfo->data() + (offset++) * size), prototype, size);
			prototype = (void*)((std::uintptr_t)prototype + size);
		}
	}
}


void command_buffer_t::remove_component(
  const details::component_key_t& key,
  psl::array_view<std::pair<entity_t::size_type, entity_t::size_type>> entities) noexcept {
	auto it = std::find_if(
	  std::begin(m_Components), std::end(m_Components), [&key](const auto& cInfo) { return cInfo->id() == key; });
	psl_assert(it != std::end(m_Components), "component info for key {} was not found", key);
	(*it)->add(entities);
	(*it)->destroy(entities);
}


void command_buffer_t::remove_component(const details::component_key_t& key,
										psl::array_view<entity_t> entities) noexcept {
	auto it = std::find_if(
	  std::begin(m_Components), std::end(m_Components), [&key](const auto& cInfo) { return cInfo->id() == key; });
	psl_assert(it != std::end(m_Components), "component info for key {} was not found", key);


	(*it)->add(entities);
	(*it)->destroy(entities);
}

// consider an alias feature
// ie: alias transform = position, rotation, scale components
void command_buffer_t::destroy(psl::array_view<entity_t> entities) noexcept {
	/*for(auto& cInfo : m_Components)
	{
		cInfo->destroy(entities);
	}*/
	// m_DestroyedEntities.insert(std::end(m_DestroyedEntities), std::begin(entities), std::end(entities));

	for(auto e : entities) {
		if(static_cast<entity_t::size_type>(e) < m_First) {
			m_DestroyedEntities.emplace_back(e);
			continue;
		}
		++m_Orphans;
		m_Entities[static_cast<entity_t::size_type>(e)] = entity_t {m_Next};
		m_Next										 = static_cast<entity_t::size_type>(e);
	}
}

void command_buffer_t::destroy(psl::ecs::details::indirect_array_t<entity_t, entity_t::size_type> entities) noexcept {
	for(auto e : entities) {
		if(static_cast<entity_t::size_type>(e) < m_First) {
			m_DestroyedEntities.emplace_back(e);
			continue;
		}
		++m_Orphans;
		m_Entities[static_cast<entity_t::size_type>(e)] = entity_t {m_Next};
		m_Next										 = static_cast<entity_t::size_type>(e);
	}
}

void command_buffer_t::destroy(entity_t entity) noexcept {
	/*for(auto& cInfo : m_Components)
	{
		cInfo->destroy(entity);
	}*/

	m_DestroyedEntities.emplace_back(entity);
	if(static_cast<entity_t::size_type>(entity) < m_First)
		return;

	m_Entities[static_cast<entity_t::size_type>(entity)] = entity_t {m_Next};
	m_Next											  = static_cast<entity_t::size_type>(entity);

	++m_Orphans;
}
