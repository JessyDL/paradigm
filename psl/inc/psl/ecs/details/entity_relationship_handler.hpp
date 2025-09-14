#pragma once
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	#include "psl/array.hpp"
	#include "psl/ecs/entity.hpp"
	#include "psl/ecs/selectors.hpp"
	#include "psl/serialization/serializer.hpp"
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
	entity_relationship_handler_t()												   = default;
	entity_relationship_handler_t(entity_relationship_handler_t const&)			   = delete;
	entity_relationship_handler_t(entity_relationship_handler_t&&)				   = delete;
	entity_relationship_handler_t& operator=(entity_relationship_handler_t const&) = delete;
	entity_relationship_handler_t& operator=(entity_relationship_handler_t&&)	   = delete;

	template <typename S>
	void serialize(S& serializer) {
		std::vector<entity_t::size_type> relationship_entities {};
		std::vector<entity_t::size_type> relationship_parents {};

		if constexpr(psl::serialization::details::IsEncoder<S>) {
			auto relationship_indices = m_ParentRelationship.indices();
			auto relationship_data	  = psl::array_view(m_ParentRelationship.begin(), m_ParentRelationship.end());

			auto relationship_indices_it = relationship_indices.begin();
			auto relationship_data_it	 = relationship_data.begin();

			for(auto end = relationship_indices.end(); relationship_indices_it != end;
				++relationship_indices_it, ++relationship_data_it) {
				relationship_entities.emplace_back(*relationship_indices_it);
				relationship_parents.emplace_back(relationship_data_it->parent.value());
			}
		}

		serializer.template parse<"REL_ENTITIES">(relationship_entities);
		serializer.template parse<"REL_PARENTS">(relationship_parents);


		if constexpr(psl::serialization::details::IsDecoder<S>) {
			auto rel_ent_it = relationship_entities.begin();
			auto rel_par_it = relationship_parents.begin();

			for(auto end = relationship_entities.end(); rel_ent_it != end; ++rel_ent_it, ++rel_par_it) {
				set_parent(details::make_entity(*rel_par_it), details::make_entity(*rel_ent_it));
			}
		}
	}

  public:
	template <typename T>
		requires(IsRangeType<T>)
	void set_parent(entity_t parent, T const& children) noexcept {
		set_parent(parent, std::to_address(children.begin()), std::to_address(children.end()));
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
	void set_parent(entity_t parent, entity_t const* begin, entity_t const* const end) noexcept;
	entity_relationship_t const* get_relationship(entity_t target) const noexcept {
		return m_ParentRelationship.try_get(target.value());
	}
	void clear() noexcept;
	auto modified_hierarchy_entities() const noexcept -> psl::array_view<entity_t const> {
		return psl::array_view<entity_t const> {(entity_t*)m_ModifiedHierarchy.indices().data(),
												(entity_t*)m_ModifiedHierarchy.indices().data() +
												  m_ModifiedHierarchy.indices().size()};
	}
	void clear_modified_hierarchy() noexcept {
		m_ModifiedHierarchy.clear();
	}

	auto modified_hierarchy_data() const noexcept -> psl::array_view<hierarchy_change_event const> {
		return psl::array_view<hierarchy_change_event const>(m_ModifiedHierarchy.begin(), m_ModifiedHierarchy.end());
	}

	auto change_event(entity_t e) const noexcept -> hierarchy_change_event const* {
		return m_ModifiedHierarchy.try_get(e.value());
	}

  private:
	psl::sparse_array<hierarchy_change_event, entity_t::size_type> m_ModifiedHierarchy {};
	psl::sparse_array<entity_relationship_t, entity_t::size_type> m_ParentRelationship {};
};
}	 // namespace psl::ecs::details
#else
	#include "psl/ecs/details/entity_relationship_null_handler.hpp"
#endif
