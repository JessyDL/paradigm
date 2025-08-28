#include "psl/ecs/details/entity_relationship_handler.hpp"
#include "psl/assertions.hpp"
#include "psl/utility/cast.hpp"

namespace psl::ecs::details {
bool entity_relationship_handler_t::has_parent(entity_t target) const noexcept {
	psl_assert(target != invalid_entity, "cannot check if an invalid entity has a parent");
	auto entry = m_ParentRelationship.try_get(details::get_value(target));
	return entry && entry->parent != invalid_entity;
}

bool entity_relationship_handler_t::has_siblings(entity_t target) const noexcept {
	psl_assert(target != invalid_entity, "cannot check if an invalid entity has siblings");
	auto entry = m_ParentRelationship.try_get(details::get_value(target));
	return entry && (entry->next_sibling != target || entry->next_sibling != invalid_entity);
}

bool entity_relationship_handler_t::has_children(entity_t target) const noexcept {
	psl_assert(target != invalid_entity, "cannot check if an invalid entity is a parent");
	auto entry = m_ParentRelationship.try_get(details::get_value(target));
	return entry && entry->first_child != invalid_entity;
}

bool entity_relationship_handler_t::is_child_of(entity_t parent, entity_t child) const noexcept {
	psl_assert(parent != invalid_entity, "cannot check if an invalid entity is a parent");
	psl_assert(child != invalid_entity, "cannot check if an invalid entity is a child");
	auto entry = m_ParentRelationship.try_get(details::get_value(child));
	return entry && entry->parent == parent;
}

bool entity_relationship_handler_t::is_parent_of(entity_t parent, entity_t child) const noexcept {
	psl_assert(parent != invalid_entity, "cannot check if an invalid entity is a parent");
	psl_assert(child != invalid_entity, "cannot check if an invalid entity is a child");
	auto entry	 = m_ParentRelationship.try_get(details::get_value(parent));
	auto current = entry ? entry->first_child : invalid_entity;
	if(current == invalid_entity) {
		return false;
	}
	do {
		if(current == child) {
			return true;
		}
		current = m_ParentRelationship.at(details::get_value(current)).next_sibling;
	} while(current != invalid_entity && current != entry->first_child);
	return false;
}

bool entity_relationship_handler_t::is_sibling(entity_t first, entity_t second) const noexcept {
	psl_assert(first != invalid_entity, "cannot check if an invalid entity is a sibling");
	psl_assert(second != invalid_entity, "cannot check if an invalid entity is a sibling");
	auto first_entry  = m_ParentRelationship.try_get(details::get_value(first));
	auto second_entry = m_ParentRelationship.try_get(details::get_value(second));
	return first_entry && second_entry && first_entry->parent == second_entry->parent &&
		   first_entry->parent != invalid_entity;
}

bool entity_relationship_handler_t::is_indirect_parent_of(entity_t parent, entity_t child) const noexcept {
	psl_assert(parent != invalid_entity, "cannot check if an invalid entity is a parent");
	psl_assert(child != invalid_entity, "cannot check if an invalid entity is a child");
	auto entry = m_ParentRelationship.at(details::get_value(child));
	while(entry.parent != invalid_entity && entry.parent != parent) {
		entry = m_ParentRelationship.at(details::get_value(entry.parent));
	}
	return entry.parent == parent;
}

bool entity_relationship_handler_t::is_root(entity_t target) const noexcept {
	psl_assert(target != invalid_entity, "cannot check if an invalid entity is a root");
	auto entry = m_ParentRelationship.try_get(details::get_value(target));
	return !entry || entry->parent == invalid_entity;
}

entity_t entity_relationship_handler_t::get_root(entity_t target) const noexcept {
	psl_assert(target != invalid_entity, "cannot check if an invalid entity is a root");
	auto previous = target;
	while(target != invalid_entity) {
		previous   = target;
		auto entry = m_ParentRelationship.try_get(details::get_value(target));
		target	   = entry ? entry->parent : invalid_entity;
	}
	return previous;
}


void entity_relationship_handler_t::set_parent(entity_t parent,
											   entity_t const* begin,
											   entity_t const* const end) noexcept {
	psl_assert(std::none_of(begin, end, [](entity_t e) { return e == invalid_entity; }),
			   "cannot set a parent to an invalid entity");
	psl_assert(std::none_of(begin, end, [parent](entity_t e) { return e == parent; }),
			   "cannot set a parent to itself, this would create a cycle in the hierarchy");

	psl::array<std::pair<entity_t, entity_relationship_t*>> children {};
	children.reserve(std::distance(begin, end));

	// needed to guarantee pointer stability for the next section.
	m_ParentRelationship.reserve(m_ParentRelationship.capacity() + std::distance(begin, end) + 1, true);

	for(auto child = begin; child != end; ++child) {
		auto& child_entry = m_ParentRelationship.at(details::get_value(*child));

		// only modify if the child is not already set to the parent
		if(child_entry.parent == parent) {
			continue;
		}

		// first we update the existing child's parent entry if it is present and then the siblings
		// erasing the current child entity from their entries.
		if(child_entry.parent != invalid_entity) {
			auto& parent_entry = m_ParentRelationship.at(details::get_value(child_entry.parent));
			parent_entry.children--;
			m_ModifiedHierarchy[details::get_value(child_entry.parent)] |= hierarchy_change_event::child_removed;
			if(parent_entry.first_child == *child) {
				parent_entry.first_child = child_entry.next_sibling;
			}
		}

		if(child_entry.next_sibling != invalid_entity) {
			auto& next_sibling_entry = m_ParentRelationship.at(details::get_value(child_entry.next_sibling));
			next_sibling_entry.prev_sibling =
			  child_entry.prev_sibling == child_entry.next_sibling ? invalid_entity : child_entry.prev_sibling;
			child_entry.next_sibling = invalid_entity;
		}
		if(child_entry.prev_sibling != invalid_entity) {
			auto& prev_sibling_entry = m_ParentRelationship.at(details::get_value(child_entry.prev_sibling));
			prev_sibling_entry.next_sibling =
			  child_entry.prev_sibling == child_entry.next_sibling ? invalid_entity : child_entry.next_sibling;
			child_entry.prev_sibling = invalid_entity;
		}


		// update the child and its dependents to the new parent
		{
			child_entry.parent = parent;
			m_ModifiedHierarchy[details::get_value(*child)] |= hierarchy_change_event::reparented;
		}

		children.emplace_back(*child, &child_entry);
	}

	// no parent or no valid children, so we don't need to do anything else
	if(parent == invalid_entity || children.empty()) {
		return;
	}

	auto& parent_entry = m_ParentRelationship.at(details::get_value(parent));
	m_ModifiedHierarchy[details::get_value(parent)] |= hierarchy_change_event::child_added;
	parent_entry.children += psl::narrow_cast<entity_t::size_type>(children.size());

	if(children.size() == 1) {
		auto& child_entry = children.front();
		if(parent_entry.first_child == invalid_entity) {
			parent_entry.first_child = child_entry.first;
		} else {
			auto& parent_first_child_entry = m_ParentRelationship.at(details::get_value(parent_entry.first_child));
			if(parent_first_child_entry.prev_sibling != invalid_entity) {
				auto& last_child_entry =
				  m_ParentRelationship.at(details::get_value(parent_first_child_entry.prev_sibling));
				last_child_entry.next_sibling		  = child_entry.first;
				child_entry.second->prev_sibling	  = parent_first_child_entry.prev_sibling;
				child_entry.second->next_sibling	  = parent_entry.first_child;
				parent_first_child_entry.prev_sibling = child_entry.first;
			} else {
				parent_first_child_entry.next_sibling = child_entry.first;
				child_entry.second->prev_sibling	  = parent_entry.first_child;
			}
		}
	} else {
		// complexer form, we'll first form a chain with the children we accumulated in the previous pass
		// then we'll inject this new chain into the parent entry.
		for(auto first = children.begin(), second = std::next(children.begin()); second != children.end();
			first = second, second = std::next(second)) {
			first->second->next_sibling	 = second->first;
			second->second->prev_sibling = first->first;
		}

		// nicest form, just link our childrens first to the last child and then set the first child of the parent.
		if(parent_entry.first_child == invalid_entity) {
			auto& first				   = children.front();
			auto& last				   = children.back();
			last.second->next_sibling  = first.first;
			first.second->prev_sibling = last.first;
			parent_entry.first_child   = first.first;
		} else {
			// less nice form, get the first and last child of the parent
			auto& first					   = children.front();
			auto& last					   = children.back();
			auto& parent_first_child_entry = m_ParentRelationship.at(details::get_value(parent_entry.first_child));

			if(parent_first_child_entry.prev_sibling != invalid_entity) {
				auto& parent_last_child_entry =
				  m_ParentRelationship.at(details::get_value(parent_first_child_entry.prev_sibling));

				parent_last_child_entry.next_sibling  = first.first;
				first.second->prev_sibling			  = parent_first_child_entry.prev_sibling;
				parent_first_child_entry.prev_sibling = last.first;
				last.second->next_sibling			  = parent_entry.first_child;
			} else {
				parent_first_child_entry.next_sibling = first.first;
				first.second->prev_sibling			  = parent_entry.first_child;
				parent_first_child_entry.prev_sibling = last.first;
				last.second->next_sibling			  = parent_entry.first_child;
			}
		}
	}
}

void entity_relationship_handler_t::set_parent(entity_t parent, entity_t child) noexcept {
	psl_assert(parent != child, "cannot set a parent to itself, this would create a cycle in the hierarchy");
	psl_assert(child != invalid_entity, "cannot set the child to an invalid value");
	auto& child_entry = m_ParentRelationship.at(details::get_value(child));
	if(child_entry.parent == parent) {
		return;	   // already set
	}

	// first we update the existing child's parent entry if it is present and then the siblings
	// erasing the current child entity from their entries.
	if(child_entry.parent != invalid_entity) {
		auto& parent_entry = m_ParentRelationship.at(details::get_value(child_entry.parent));
		parent_entry.children--;
		m_ModifiedHierarchy[details::get_value(child_entry.parent)] |= hierarchy_change_event::child_removed;
		if(parent_entry.first_child == child) {
			parent_entry.first_child = child_entry.next_sibling;
		}
	}

	if(child_entry.next_sibling != invalid_entity) {
		auto& next_sibling_entry = m_ParentRelationship.at(details::get_value(child_entry.next_sibling));
		next_sibling_entry.prev_sibling =
		  child_entry.prev_sibling == child_entry.next_sibling ? invalid_entity : child_entry.prev_sibling;
	}
	if(child_entry.prev_sibling != invalid_entity) {
		auto& prev_sibling_entry = m_ParentRelationship.at(details::get_value(child_entry.prev_sibling));
		prev_sibling_entry.next_sibling =
		  child_entry.prev_sibling == child_entry.next_sibling ? invalid_entity : child_entry.next_sibling;
	}


	// update the child and its dependents to the new parent
	{
		child_entry.parent = parent;

		// reset the siblings value, if the child gets a new parent these will be filled in again
		// later in the scope
		child_entry.next_sibling = invalid_entity;
		child_entry.prev_sibling = invalid_entity;

		m_ModifiedHierarchy[details::get_value(child)] |= hierarchy_change_event::reparented;
	}

	if(parent == invalid_entity) {
		return;	   // no parent, so we don't need to do anything else
	}
	auto& parent_entry = m_ParentRelationship.at(details::get_value(parent));
	parent_entry.children++;
	m_ModifiedHierarchy[details::get_value(parent)] |= hierarchy_change_event::child_added;
	// if the parent had no children, then we can simply set the current child as the first child.
	// otherwise we need to fetch the first child, update its prev_sibling value and set that to the
	// newly added child.
	// additionally we fetch the last child and set the next_sibling of the last child to the newly added child.
	if(parent_entry.first_child == invalid_entity) {
		parent_entry.first_child = child;
	} else {
		auto& parent_first_child_entry = m_ParentRelationship.at(details::get_value(parent_entry.first_child));
		if(parent_first_child_entry.prev_sibling != invalid_entity) {
			auto& last_child_entry = m_ParentRelationship.at(details::get_value(parent_first_child_entry.prev_sibling));
			last_child_entry.next_sibling = child;
			child_entry.prev_sibling	  = parent_first_child_entry.prev_sibling;
		} else {
			parent_first_child_entry.next_sibling = child;
			child_entry.prev_sibling			  = parent_entry.first_child;
		}
		parent_first_child_entry.prev_sibling = child;
		child_entry.next_sibling			  = parent_entry.first_child;
	}
}

void entity_relationship_handler_t::unparent(entity_t target) {
	set_parent(invalid_entity, target);
}

psl::array<entity_t> entity_relationship_handler_t::get_children(entity_t parent, bool direct_only) const noexcept {
	psl::array<entity_t> result {};
	auto parent_entry = m_ParentRelationship.try_get(details::get_value(parent));
	if(!parent_entry || parent_entry->first_child == invalid_entity) {
		return result;	  // no children
	}
	result.reserve(parent_entry->children);
	auto const first = parent_entry->first_child;
	auto current	 = first;
	do {
		result.emplace_back(current);
		auto& child_entry = m_ParentRelationship.at(details::get_value(current));
		current			  = child_entry.next_sibling;
	} while(current != invalid_entity && current != first);
	if(!direct_only) {
		const auto count = result.size();
		for(auto i = 0u; i < count; ++i) {
			auto children = get_children(result[i], false);
			result.insert(std::end(result), std::begin(children), std::end(children));
		}
	}
	return result;
}

psl::array<entity_t> entity_relationship_handler_t::get_direct_children(entity_t parent) const noexcept {
	return get_children(parent, true);
}

psl::array<entity_t> entity_relationship_handler_t::get_all_children(entity_t parent) const noexcept {
	return get_children(parent, false);
}

psl::array<entity_t> entity_relationship_handler_t::get_all_parents(entity_t child) const noexcept {
	psl::array<entity_t> result {};
	auto current = child;
	while(current != invalid_entity) {
		auto entry = m_ParentRelationship.try_get(details::get_value(current));
		if(!entry || entry->parent == invalid_entity) {
			break;	  // no parent
		}
		result.emplace_back(entry->parent);
		current = entry->parent;
	}
	return result;
}

entity_t entity_relationship_handler_t::get_parent(entity_t child) const noexcept {
	auto entry = m_ParentRelationship.try_get(details::get_value(child));
	return entry ? entry->parent : invalid_entity;
}

psl::array<entity_t> entity_relationship_handler_t::get_siblings(entity_t target) const noexcept {
	psl_assert(target != invalid_entity, "cannot get siblings of an invalid entity");
	auto entry	 = m_ParentRelationship.try_get(details::get_value(target));
	auto current = entry ? entry->next_sibling : invalid_entity;
	psl::array<entity_t> result {};
	while(current != invalid_entity && current != target) {
		result.push_back(current);
		entry	= m_ParentRelationship.try_get(details::get_value(current));
		current = entry ? entry->next_sibling : invalid_entity;
	}
	return result;
}

void entity_relationship_handler_t::clear() noexcept {
	m_ParentRelationship.clear();
	m_ModifiedHierarchy.clear();
}

}	 // namespace psl::ecs::details
