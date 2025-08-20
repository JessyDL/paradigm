#include "psl/ecs/entity_relationship_data.hpp"

namespace psl::ecs {
entity_relationship_data_t::entity_relationship_data_t()  = default;
entity_relationship_data_t::~entity_relationship_data_t() = default;

entity_relationship_data_t::entity_relationship_data_t(entity_relationship_data_t const& rhs)
	: m_Parent(rhs.m_Parent), m_Self(rhs.m_Self), m_Children(rhs.m_Children), m_Siblings(rhs.m_Siblings) {}

entity_relationship_data_t::entity_relationship_data_t(entity_relationship_data_t&& rhs)
	: m_Parent(std::move(rhs.m_Parent)), m_Self(std::move(rhs.m_Self)), m_Children(std::move(rhs.m_Children)),
	  m_Siblings(std::move(rhs.m_Siblings)) {}

entity_relationship_data_t& entity_relationship_data_t::operator=(entity_relationship_data_t const& rhs) {
	if(this != &rhs) {
		m_Parent   = rhs.m_Parent;
		m_Self	   = rhs.m_Self;
		m_Children = rhs.m_Children;
		m_Siblings = rhs.m_Siblings;
	}
	return *this;
}

entity_relationship_data_t& entity_relationship_data_t::operator=(entity_relationship_data_t&& rhs) {
	if(this != &rhs) {
		m_Parent   = std::move(rhs.m_Parent);
		m_Self	   = std::move(rhs.m_Self);
		m_Children = std::move(rhs.m_Children);
		m_Siblings = std::move(rhs.m_Siblings);
	}
	return *this;
}

entity_t entity_relationship_data_t::parent() const noexcept {
	return m_Parent;
}

bool entity_relationship_data_t::is_root() const noexcept {
	return m_Parent == invalid_entity;
}

bool entity_relationship_data_t::has_children() const noexcept {
	return m_Children && !m_Children->empty();
}

bool entity_relationship_data_t::has_siblings() const noexcept {
	return m_Siblings && m_Siblings->size() > 1;
}

bool entity_relationship_data_t::has_parent() const noexcept {
	return m_Parent != invalid_entity;
}

size_t entity_relationship_data_t::children_count() const noexcept {
	if(m_Children) {
		return m_Children->size();
	}
	return 0;
}

size_t entity_relationship_data_t::siblings_count() const noexcept {
	if(m_Siblings) {
		return m_Siblings->size() - 1;
	}
	return 0;
}

psl::array_view<entity_t const> entity_relationship_data_t::children() const noexcept {
	if(m_Children) {
		return psl::array_view<entity_t const>(m_Children->data(), m_Children->size());
	}
	return psl::array_view<entity_t const> {};
}

psl::array_view<entity_t const> entity_relationship_data_t::siblings() const noexcept {
	if(m_Siblings) {
		return psl::array_view<entity_t const>(m_Siblings->data(), m_Siblings->size());
	}
	return psl::array_view<entity_t const> {(const entity_t*)&m_Self, ((const entity_t*)&m_Self) + 1};
}
psl::array<entity_t> entity_relationship_data_t::siblings_excluding_self() const noexcept {
	if(m_Siblings) {
		psl::array<entity_t> siblings_ {m_Siblings->begin(), m_Siblings->end()};
		auto it = std::find(siblings_.begin(), siblings_.end(), m_Self);
		if(it != siblings_.end()) {
			// remove the self from the siblings
			*it = siblings_.back();
			siblings_.pop_back();
		}
		return siblings_;
	}
	return {};
}

}	 // namespace psl::ecs
