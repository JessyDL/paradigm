#pragma once
#include "psl/array.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/ecs/selectors.hpp"
#include "psl/sparse_array.hpp"

namespace psl::ecs::details {
class entity_relationship_handler_t {
  protected:
	struct entity_relationship_t {
		entity_t parent {};
		entity_t first_child {};
		entity_t next_sibling {};
		entity_t prev_sibling {};
		entity_t::size_type children {0};	 // number of direct children this entity has
	};

  public:
	template <typename T>
		requires(IsRangeType<T>)
	void set_parent(entity_t parent, T const& children) noexcept {
		for(auto child : children) {
			set_parent(parent, child);
		}
	}

	bool has_parent(entity_t target) const noexcept;
	bool has_siblings(entity_t target) const noexcept;
	bool has_children(entity_t target) const noexcept;
	bool is_child_of(entity_t parent, entity_t child) const noexcept;
	bool is_parent_of(entity_t parent, entity_t child) const noexcept;
	bool is_sibling(entity_t first, entity_t second) const noexcept;
	bool is_indirect_parent_of(entity_t parent, entity_t child) const noexcept;
	bool is_root(entity_t target) const noexcept;
	entity_t get_root(entity_t target) const noexcept;
	void set_parent(entity_t parent, entity_t child) noexcept;
	void unparent(entity_t target);
	psl::array<entity_t> get_children(entity_t parent, bool direct_only = false) const noexcept;
	psl::array<entity_t> get_direct_children(entity_t parent) const noexcept;
	psl::array<entity_t> get_all_children(entity_t parent) const noexcept;
	psl::array<entity_t> get_all_parents(entity_t child) const noexcept;
	entity_t get_parent(entity_t child) const noexcept;
	psl::array<entity_t> get_siblings(entity_t target) const noexcept;

  protected:
	psl::sparse_array<hierarchy_change_event, entity_t::size_type> m_ModifiedHierarchy {};
	psl::sparse_array<entity_relationship_t, entity_t::size_type> m_ParentRelationship {};
};
}	 // namespace psl::ecs::details
