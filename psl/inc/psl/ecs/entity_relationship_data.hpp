#pragma once
#include "psl/ecs/entity.hpp"
#include <memory>
#include <psl/array.hpp>
#include <psl/array_view.hpp>
#include <psl/ecs/component_traits.hpp>

namespace psl::ecs {
class state_t;

class entity_relationship_data_t final {
	friend class state_t;
	entity_relationship_data_t(entity_t self) : m_Self(self) {}

  public:
	entity_relationship_data_t();
	~entity_relationship_data_t();
	entity_relationship_data_t(entity_relationship_data_t const& rhs);
	entity_relationship_data_t(entity_relationship_data_t&& rhs);
	entity_relationship_data_t& operator=(entity_relationship_data_t const& rhs);
	entity_relationship_data_t& operator=(entity_relationship_data_t&& rhs);

	entity_t parent() const noexcept;
	bool is_root() const noexcept;
	bool has_children() const noexcept;
	bool has_siblings() const noexcept;
	bool has_parent() const noexcept;
	size_t children_count() const noexcept;
	size_t siblings_count() const noexcept;
	psl::array_view<entity_t const> children() const noexcept;

	/// \brief Returns the siblings of this entity including itself.
	psl::array_view<entity_t const> siblings() const noexcept;

	/// \brief Returns the siblings of this entity excluding itself, but to do that it will create a new array.
	[[nodiscard]] psl::array<entity_t> siblings_excluding_self() const noexcept;

  private:
	entity_t m_Parent {};
	entity_t m_Self {};
	std::shared_ptr<psl::array<entity_t>> m_Children {};
	std::shared_ptr<psl::array<entity_t>> m_Siblings {};
};

template <>
struct component_trait_mutability_t<entity_relationship_data_t> {
	static constexpr component_mutability_behaviour_t mutability {component_mutability_behaviour_t::restricted};
};
}	 // namespace psl::ecs
