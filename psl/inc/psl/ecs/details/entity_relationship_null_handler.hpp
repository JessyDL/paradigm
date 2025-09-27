#pragma once
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/ecs/selectors.hpp"

namespace psl::ecs::details {

/// \brief A null implementation of the entity relationship handler.
///
/// When disabling the entity relationship handler, this class is used as a placeholder for all calls.
/// By default all its methods are empty and return default values. Additionally its entire interface is
/// protected so that the state_t can call it, but external users cannot access it anymore.
class entity_relationship_handler_t {
  public:
	struct entity_relationship_t {};

  protected:
	entity_relationship_handler_t()												   = default;
	entity_relationship_handler_t(entity_relationship_handler_t const&)			   = delete;
	entity_relationship_handler_t(entity_relationship_handler_t&&)				   = delete;
	entity_relationship_handler_t& operator=(entity_relationship_handler_t const&) = delete;
	entity_relationship_handler_t& operator=(entity_relationship_handler_t&&)	   = delete;

	template <typename S>
	void serialize(S& serializer) {}

	template <typename T>
		requires(IsRangeType<T>)
	void set_parent(entity_t parent, T const& children) noexcept {}

	bool has_parent(entity_t target) const noexcept {
		return false;
	}
	bool has_siblings(entity_t target) const noexcept {
		return false;
	}
	bool has_children(entity_t target) const noexcept {
		return false;
	}
	bool is_child_of(entity_t parent, entity_t child) const noexcept {
		return false;
	}
	bool is_parent_of(entity_t parent, entity_t child) const noexcept {
		return false;
	}
	bool is_sibling(entity_t first, entity_t second) const noexcept {
		return false;
	}
	bool is_indirect_parent_of(entity_t parent, entity_t child) const noexcept {
		return false;
	}
	bool is_root(entity_t target) const noexcept {
		return true;
	}
	entity_t get_root(entity_t target) const noexcept {
		return invalid_entity;
	}
	void set_parent(entity_t parent, entity_t child) noexcept {}
	void unparent(entity_t target) {}
	psl::array<entity_t> get_children(entity_t parent, bool direct_only = false) const noexcept {
		return {};
	}
	psl::array<entity_t> get_direct_children(entity_t parent) const noexcept {
		return {};
	}
	psl::array<entity_t> get_all_children(entity_t parent) const noexcept {
		return {};
	}
	psl::array<entity_t> get_all_parents(entity_t child) const noexcept {
		return {};
	}
	entity_t get_parent(entity_t child) const noexcept {
		return invalid_entity;
	}
	psl::array<entity_t> get_siblings(entity_t target) const noexcept {
		return {};
	}

	void set_parent(entity_t parent, entity_t const* begin, entity_t const* const end) noexcept {}
	entity_relationship_t const* get_relationship(entity_t target) const noexcept {
		return nullptr;
	}
	void clear() noexcept {}
	auto modified_hierarchy_entities() const noexcept -> psl::array_view<entity_t const> {
		return {};
	}
	void clear_modified_hierarchy() noexcept {}

	auto modified_hierarchy_data() const noexcept -> psl::array_view<hierarchy_change_event const> {
		return {};
	}

	auto change_event(entity_t e) const noexcept -> hierarchy_change_event const* {
		return nullptr;
	}
};
}	 // namespace psl::ecs::details
