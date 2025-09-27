#pragma once
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/ecs/details/component_key.hpp"
#include "psl/ecs/details/entity_relationship_handler.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/ecs/filtering.hpp"
#include "psl/ecs/selectors.hpp"

namespace psl::ecs::details {
class component_container_t;
class filter_handler_t {
  public:
	virtual ~filter_handler_t()																		   = default;
	virtual void initialize(details::filter_result& data) const noexcept							   = 0;
	virtual void filter(details::filter_result& data, psl::array_view<entity_t> source) const noexcept = 0;
	virtual psl::array<entity_t>::iterator filter(details::filter_group const& group,
												  psl::array<entity_t>::iterator begin,
												  psl::array<entity_t>::iterator end) const noexcept   = 0;
	virtual psl::array<details::filter_result>& get_filter_results() noexcept						   = 0;
};
}	 // namespace psl::ecs::details
