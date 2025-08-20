#include "psl/ecs/details/entity_container.hpp"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <stdexcept>

#include "psl/assertions.hpp"

namespace psl::ecs::details {

entity_container_t::entity_container_t() {
	m_ModifiedEntities.reserve(1 << 16);
}
entity_t entity_container_t::create() {
	entity_t entity {};
	if(!m_Orphans.empty()) {
		entity = m_Orphans.back();
		m_Orphans.pop_back();
	} else {
		entity = m_Entities++;
	}
	return entity;
}

psl::array<entity_t> entity_container_t::create(entity_t::size_type count) {
	const auto recycled = std::min<entity_t::size_type>(count, psl::narrow_cast<entity_t::size_type>(m_Orphans.size()));
	const auto remainder = count - recycled;

	psl::array<entity_t> entities(count);
	if(recycled > 0) {
		auto it = std::prev(std::end(m_Orphans));
		for(entity_t::size_type i = 0; i != recycled; ++i) {
			entities[i] = *it;
			it			= std::prev(it);
		}
		m_Orphans.erase(it, std::end(m_Orphans));
	}

	static_assert(sizeof(entity_t) == sizeof(entity_t::size_type),
				  "entity_t::size_type must be the same size as entity_t for this to work correctly.");
	std::iota((entity_t::size_type*)entities.data() + recycled,
			  (entity_t::size_type*)entities.data() + entities.size(),
			  m_Entities);
	m_Entities += remainder;
	return entities;
}

void entity_container_t::destroy(psl::array_view<entity_t> entities) noexcept {
	psl::array_view<entity_t::size_type> entity_values((entity_t::size_type*)entities.data(), entities.size());
	m_ModifiedEntities.try_insert(entity_values.begin(), entity_values.end());
	m_ToBeOrphans.insert(std::end(m_ToBeOrphans),
						 std::make_move_iterator(std::begin(entities)),
						 std::make_move_iterator(std::end(entities)));
}

void entity_container_t::destroy(entity_t entity) noexcept {
	psl_assert(entity.valid(), "attempting to destroy invalid entity");
	if(!entity.valid()) {
		return;
	}
	m_ToBeOrphans.emplace_back(entity);
	m_ModifiedEntities.try_insert(entity.value());
}

psl::array<entity_t> entity_container_t::all_entities() const noexcept {
	auto orphans = m_Orphans;
	auto count	 = orphans.end() - orphans.begin();
	if(count > 0) {
		auto* data = (entity_t::size_type*)orphans.data();
		std::sort(data, data + count);
	}

	auto orphan_it = std::begin(orphans);
	psl::array<entity_t> result;
	result.reserve(m_Entities - orphans.size());
	for(entity_t::size_type e = 0; e < m_Entities; ++e) {
		if(orphan_it != std::end(orphans) && e == orphan_it->value()) {
			orphan_it = std::next(orphan_it);
			continue;
		}
		result.emplace_back(details::make_entity(e));
	}
	return result;
}

void entity_container_t::modify_entities(psl::array_view<entity_t> entities) noexcept {
	psl::array_view<entity_t::size_type> entity_values((entity_t::size_type*)&*entities.begin(),
													   (entity_t::size_type*)&*entities.end());
	modify_entities(entity_values);
}
void entity_container_t::modify_entities(psl::array_view<entity_t::size_type> entities) noexcept {
	m_ModifiedEntities.try_insert(entities.begin(), entities.end());
}

void entity_container_t::modify_entity(entity_t entity) noexcept {
	m_ModifiedEntities.try_insert(entity.value());
}

void entity_container_t::process_to_be_orphans() noexcept {
	m_Orphans.insert(std::end(m_Orphans), std::begin(m_ToBeOrphans), std::end(m_ToBeOrphans));
	m_ToBeOrphans.clear();
}

void entity_container_t::clear() noexcept {
	m_Entities = 0;
	m_Orphans.clear();
	m_ToBeOrphans.clear();
	m_ModifiedEntities.clear();
}
}	 // namespace psl::ecs::details
