#include "psl/ecs/filtering.hpp"
#include "psl/ecs/details/components_cache.hpp"
#include "psl/ecs/details/entity_relationship_handler.hpp"
#include "psl/ecs/details/stage_range.hpp"
#include "psl/ecs/entity_relationship_data.hpp"
#include "psl/ecs/state.hpp"

#include "tracy/Tracy.hpp"

namespace psl::ecs::details {
cached_container_entry_t::cached_container_entry_t(details::component_container_t* target)
	: key(target->id()), container(target) {};
cached_container_entry_t::cached_container_entry_t(const details::component_key_t& target_key,
												   details::component_container_t* target_container)
	: key(target_key), container(target_container) {
	psl_assert(target_container == nullptr || target_key == target_container->id(),
			   "ID did not match. Was expecting '{}' but got '{}'",
			   target_key.name(),
			   target_container->id().name());
};
cached_container_entry_t::cached_container_entry_t(const details::component_key_t& target_key,
												   const details::component_key_t& container_key,
												   details::component_container_t* target_container)
	: key(target_key), container(target_container) {
	psl_assert(target_container == nullptr || container_key == target_container->id(),
			   "ID did not match. Was expecting '{}' but got '{}'",
			   container_key.name(),
			   target_container->id().name());
};

psl::array<entity_t>::iterator filter_op(details::cached_container_entry_t const& entry,
										 psl::array<entity_t>::iterator begin,
										 psl::array<entity_t>::iterator end,
										 details::stage_range_t range = details::stage_range_t::ALIVE) noexcept {
	ZoneScoped;
	return (entry.container == nullptr)
			 ? begin
			 : std::partition(begin, end, [&entry, &range](entity_t e) { return entry.container->has(e, range); });
}

psl::array<entity_t>::iterator on_add_op(details::cached_container_entry_t const& entry,
										 psl::array<entity_t>::iterator begin,
										 psl::array<entity_t>::iterator end) noexcept {
	ZoneScoped;
	return (entry.container == nullptr)
			 ? begin
			 : std::partition(begin, end, [&entry](entity_t e) { return entry.container->has_added(e); });
}

psl::array<entity_t>::iterator on_remove_op(details::cached_container_entry_t const& entry,
											psl::array<entity_t>::iterator begin,
											psl::array<entity_t>::iterator end) noexcept {
	ZoneScoped;
	return (entry.container == nullptr)
			 ? begin
			 : std::partition(begin, end, [&entry](entity_t e) { return entry.container->has_removed(e); });
}

psl::array<entity_t>::iterator on_except_op(details::cached_container_entry_t const& entry,
											psl::array<entity_t>::iterator begin,
											psl::array<entity_t>::iterator end,
											details::stage_range_t range = details::stage_range_t::ALIVE) noexcept {
	ZoneScoped;
	return (entry.container == nullptr)
			 ? end
			 : std::partition(begin, end, [&entry, &range](entity_t e) { return !entry.container->has(e, range); });
}

psl::array<entity_t>::iterator on_break_op(psl::array<details::cached_container_entry_t> const& entries,
										   psl::array<entity_t>::iterator begin,
										   psl::array<entity_t>::iterator end) noexcept {
	ZoneScoped;
	return (std::any_of(entries.begin(), entries.end(), [](const auto& cache) { return cache.container == nullptr; }))
			 ? begin
			 :
			 // for every entity, remove if...
			 std::partition(begin, end, [&entries](entity_t e) {
				 return
				   // any of them have not had an entity removed
				   !(!std::any_of(std::begin(entries),
								  std::end(entries),
								  [e](const auto& entry) { return entry.container->has_removed(e); }) ||
					 // or all of them do not have a component, or had the entity removed
					 !std::all_of(std::begin(entries), std::end(entries), [e](const auto& entry) {
						 return entry.container->has_component(e) || entry.container->has_removed(e);
					 }));
			 });
}

psl::array<entity_t>::iterator on_combine_op(psl::array<details::cached_container_entry_t> const& entries,
											 psl::array<entity_t>::iterator begin,
											 psl::array<entity_t>::iterator end) noexcept {
	ZoneScoped;
	return (std::any_of(entries.begin(), entries.end(), [](const auto& cache) { return cache.container == nullptr; }))
			 ? begin
			 : std::remove_if(begin, end, [entries](entity_t e) {
				   return !std::any_of(std::begin(entries), std::end(entries), [e](const auto& entry) {
					   return entry.container->has_added(e);
				   }) || !std::all_of(std::begin(entries), std::end(entries), [e](const auto& entry) {
					   return entry.container->has_component(e);
				   });
			   });
}


psl::array<entity_t>::iterator
on_mutate_op(std::function<details::component_container_t*(component_key_t const&)> const& get_component_container,
			 details::cached_container_entry_t const& entry,
			 psl::array<entity_t>::iterator begin,
			 psl::array<entity_t>::iterator end) noexcept {
	ZoneScoped;
	auto cInfoTarget = entry.container ? get_component_container(entry.container->component_type_info().id) : nullptr;
	psl_assert(entry.container == nullptr || cInfoTarget != entry.container,
			   "The mutation data source and destination are the same container, this means we have an incorrect "
			   "lookup happening.");
	return (entry.container == nullptr || cInfoTarget == nullptr)
			 ? begin
			 : std::partition(begin, end, [&entry, &cInfoTarget](entity_t e) {
				   return entry.container->has(e) && cInfoTarget->has(e);
			   });
}

psl::array<entity_t>::iterator
on_hierarchy_op(std::function<hierarchy_change_event const*(entity_t)> const& change_event,
				hierarchy_change_event change,
				psl::array<entity_t>::iterator begin,
				psl::array<entity_t>::iterator end) noexcept {
	ZoneScoped;
	if(change == hierarchy_change_event::none) {
		return begin;
	}

	return std::partition(begin, end, [change, &change_event](entity_t e) {
		if(auto it = change_event(e); it != nullptr) {
			return (*it & change) != hierarchy_change_event::none;
		}
		return false;
	});
}

psl::array<entity_t>::iterator on_hierarchy_with_preseed_op(
  std::function<entity_relationship_handler_t::entity_relationship_t const*(entity_t)> const& get_relationship,
  hierarchy_change_event change,
  psl::array<entity_t>::iterator begin,
  psl::array<entity_t>::iterator end) noexcept {
	ZoneScoped;
	if(change == hierarchy_change_event::none) {
		return begin;
	}
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	return std::partition(begin, end, [change, &get_relationship](entity_t e) {
		if(auto relationship = get_relationship(e); relationship) {
			if((change & hierarchy_change_event::reparented) == hierarchy_change_event::reparented &&
			   relationship->parent != invalid_entity) {
				return true;
			}
			if((change & hierarchy_change_event::child_added) == hierarchy_change_event::child_added &&
			   relationship->first_child != invalid_entity) {
				return true;
			}
		}
		return false;
	});
#else
	return begin;
#endif
}

psl::array<entity_t>::iterator filter_group::execute(
  std::function<details::component_container_t*(component_key_t const&)> const& get_component_container,
  psl::array<entity_t>::iterator begin,
  psl::array<entity_t>::iterator end) const noexcept {
	auto const range = on_break.size() > 0 ? details::stage_range_t::ALL : details::stage_range_t::ALIVE;
	for(auto filter : on_mutate) {
		if(!filter.container) {
			filter.container = get_component_container(filter.key);
		}
		end = on_mutate_op(get_component_container, filter, begin, end);
	}

	for(auto filter : on_remove) {
		if(!filter.container) {
			filter.container = get_component_container(filter.key);
		}
		end = on_remove_op(filter, begin, end);
	}
	if(on_break.size() > 0) {
		std::for_each(on_break.begin(), on_break.end(), [&get_component_container](auto& filter) {
			if(!filter.container) {
				filter.container = get_component_container(filter.key);
			}
		});
		end = on_break_op(on_break, begin, end);
	}

	if(is_transient()) {
		for(auto filter : on_add) {
			if(!filter.container) {
				filter.container = get_component_container(filter.key);
			}
			end = filter_op(filter, begin, end);
		}
		for(auto filter : on_combine) {
			if(!filter.container) {
				filter.container = get_component_container(filter.key);
			}
			end = filter_op(filter, begin, end);
		}
	} else {
		for(auto filter : on_add) {
			if(!filter.container) {
				filter.container = get_component_container(filter.key);
			}
			end = on_add_op(filter, begin, end);
		}
		if(on_combine.size() > 0) {
			std::for_each(on_combine.begin(), on_combine.end(), [&get_component_container](auto& filter) {
				if(!filter.container) {
					filter.container = get_component_container(filter.key);
				}
			});
			end = on_combine_op(on_combine, begin, end);
		}
	}
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	static constexpr auto erd_key = component_key_t::generate<entity_relationship_data_t>();
#endif
	for(auto filter : filters) {
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
		if(filter.key == erd_key) {
			continue;
		}
#endif
		if(!filter.container) {
			filter.container = get_component_container(filter.key);
		}
		end = filter_op(filter, begin, end, range);
	}
	for(auto filter : except) {
		if(!filter.container) {
			filter.container = get_component_container(filter.key);
		}
		end = on_except_op(filter, begin, end, range);
	}

	return end;
}

psl::array_view<entity_t> filter_group::get_smallest(
  std::function<details::component_container_t*(component_key_t const&)> const& get_component_container)
  const noexcept {
	ZoneScoped;
	std::optional<psl::array_view<entity_t>> source;

	for(auto filter : on_mutate) {
		auto cInfo = filter.container ? filter.container : get_component_container(filter);
		if(!cInfo) {
			return {};
		}
		if(!source || cInfo->entities().size() < source.value().size()) {
			// technically remove ops on the on_mutate should not be possible, but we'll filter for all anyway.
			source = cInfo->entities(true);
		}
	}

	for(auto filter : on_remove) {
		auto cInfo = filter.container ? filter.container : get_component_container(filter);
		if(!cInfo) {
			return {};
		}
		if(!source || cInfo->removed_entities().size() < source.value().size()) {
			source = cInfo->removed_entities();
		}
	}
	for(auto filter : on_break) {
		auto cInfo = filter.container ? filter.container : get_component_container(filter);
		if(!cInfo) {
			return {};
		}
		if(!source || cInfo->entities(true).size() < source.value().size()) {
			source = cInfo->entities(true);
		}
	}
	for(auto filter : on_add) {
		auto cInfo = filter.container ? filter.container : get_component_container(filter);
		if(!cInfo) {
			return {};
		}
		if(seed_with_previous) {
			if(!source || cInfo->entities().size() < source.value().size()) {
				source = cInfo->entities();
			}
		} else {
			if(!source || cInfo->added_entities().size() < source.value().size()) {
				source = cInfo->added_entities();
			}
		}
	}
	for(auto filter : on_combine) {
		auto cInfo = filter.container ? filter.container : get_component_container(filter);
		if(!cInfo) {
			return {};
		}
		if(!source || cInfo->entities().size() < source.value().size()) {
			source = cInfo->entities();
		}
	}

	for(auto filter : filters) {
		auto cInfo = filter.container ? filter.container : get_component_container(filter);
		if(!cInfo) {
			return {};
		}
		if(!source || cInfo->entities().size() < source.value().size()) {
			source = cInfo->entities();
		}
	}

	return source.value_or(psl::array_view<entity_t> {});
}

psl::array<entity_t> get_all_relationships_unfiltered(psl::ecs::details::entity_relationship_handler_t const& state,
													  psl::array_view<entity_t> source,
													  entity_relationship relationship) noexcept {
	ZoneScoped;
	auto begin = std::begin(source);
	auto end   = std::end(source);

	psl::array<entity_t> relationship_entities {};
	if((relationship & entity_relationship::all_parents) == entity_relationship::all_parents) {
		for(auto it = begin; it != end; it = std::next(it)) {
			auto parents = state.get_all_parents(*it);
			relationship_entities.insert(std::end(relationship_entities), std::begin(parents), std::end(parents));
		}
	} else if((relationship & entity_relationship::direct_parent) == entity_relationship::direct_parent) {
		for(auto it = begin; it != end; it = std::next(it)) {
			relationship_entities.emplace_back(state.get_parent(it));
		}
	}

	if((relationship & entity_relationship::all_children) == entity_relationship::all_children) {
		for(auto it = begin; it != end; it = std::next(it)) {
			auto children = state.get_all_children(*it);
			relationship_entities.insert(std::end(relationship_entities), std::begin(children), std::end(children));
		}
	} else if((relationship & entity_relationship::direct_children) == entity_relationship::direct_children) {
		for(auto it = begin; it != end; it = std::next(it)) {
			auto children = state.get_children(*it, true);
			relationship_entities.insert(std::end(relationship_entities), std::begin(children), std::end(children));
		}
	}

	if((relationship & entity_relationship::siblings) == entity_relationship::siblings) {
		for(auto it = begin; it != end; it = std::next(it)) {
			auto siblings = state.get_siblings(*it);
			relationship_entities.insert(std::end(relationship_entities), std::begin(siblings), std::end(siblings));
		}
	}

	if((relationship & entity_relationship::self) == entity_relationship::self) {
		relationship_entities.insert(std::end(relationship_entities), std::begin(source), std::end(source));
	}

	return relationship_entities;
}

void filter_result::initialize(
  const state_t& state,
  std::function<details::component_container_t*(component_key_t const&)> const& get_component_container,
  std::function<hierarchy_change_event const*(entity_t)> const& change_event,
  std::function<entity_relationship_handler_t::entity_relationship_t const*(entity_t)> const&
	get_relationship) noexcept {
	if(group->should_be_preseeded()) {
		auto source = group->get_smallest(get_component_container);

#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
		if(group->is_hierarchy_seed_with_previous()) {
			auto cpy = psl::array<entity_t>(source.begin(), source.end());
			cpy.erase(std::remove_if(cpy.begin(),
									 cpy.end(),
									 [&get_relationship, &group = *group](entity_t e) {
										 if(auto relationship = get_relationship(e); relationship) {
											 if((group.hierarchy_change & hierarchy_change_event::reparented) ==
												  hierarchy_change_event::reparented &&
												relationship->parent != invalid_entity) {
												 return false;
											 }
											 if((group.hierarchy_change & hierarchy_change_event::child_added) ==
												  hierarchy_change_event::child_added &&
												relationship->first_child != invalid_entity) {
												 return false;
											 }
										 }
										 return true;
									 }),
					  cpy.end());

			auto cached				= group->hierarchy_change;
			group->hierarchy_change = hierarchy_change_event::none;
			execute(psl::array_view<entity_t>(cpy.begin(), cpy.end()),
					state,
					get_component_container,
					change_event,
					get_relationship);
			if(direct_entities.has_value()) {
				entities		= std::move(direct_entities.value());
				direct_entities = std::nullopt;	   // clear the direct entities as we no longer need them
			}
			group->hierarchy_change = cached;
		} else {
			execute(source, state, get_component_container, change_event, get_relationship);
		}
#else
		execute(source, state, get_component_container, change_event, get_relationship);
#endif
	}
}

void filter_result::execute(
  psl::array_view<entity_t> source,
  const state_t& state,
  std::function<details::component_container_t*(component_key_t const&)> const& get_component_container,
  std::function<hierarchy_change_event const*(entity_t)> const& change_event,
  std::function<entity_relationship_handler_t::entity_relationship_t const*(entity_t)> const&
	get_relationship) noexcept {
	ZoneScoped;
	// reset the transformations state
	for(auto& transformation : transformations) {
		transformation.should_generate = true;
	}
	if(direct_entities.has_value() && direct_entities->size() > 0) {
		// if we have direct entities, we can use those as the source
		entities		= direct_entities.value();
		direct_entities = std::nullopt;	   // clear the direct entities as we no longer need them
	}

	if(source.size() == 0) {
		if(!group->is_transient() && group->clear_every_frame()) {
			entities.clear();
		}
	} else {
		psl::array<entity_t> result {source};

		// first do the hierarchy change events, as the parents need to satisfy the filters as well and can add to
		// the source entities we can cache the resultant query of the modified entities for subsequent filters
		auto begin = std::begin(result);
		auto end   = std::end(result);

		end = group->execute(
		  [&get_component_container](component_key_t const& key) { return get_component_container(key); }, begin, end);

		if(group->is_hierarchy_change_active()) {
			if(group->is_hierarchy_seed_with_previous()) {
				end = on_hierarchy_with_preseed_op(get_relationship, group->hierarchy_change, begin, end);
			} else {
				end = on_hierarchy_op(change_event, group->hierarchy_change, begin, end);
			}
		}

		std::sort(begin, end);
		if(group->clear_every_frame() && !group->is_transient()) {
			result.erase(end, std::end(result));
			entities = std::move(result);

			// do normal operations here, we cannot save perf
			for(auto& transformation : transformations) {
				if(!transformation.group || !transformation.group->order_by ||
				   !transformation.group->on_condition.empty()) {
					continue;
				}

				transformation.entities = entities;
				transformation.entities.erase(transformation.group->transform(std::begin(transformation.entities),
																			  std::end(transformation.entities),
																			  state),
											  std::end(transformation.entities));

				transformation.should_generate = false;
			}
		} else {
			// invoke<entity_t::size_type>([](auto... args) { std::sort(args...); }, end, std::end(result));
			//
			// todo support order_by and on_condition
			// if(false && transformations.size() > 0)
			//{
			//	for (auto& transformation : transformations)
			//	{
			//		continue;
			//		psl::array<entity_t> ordered_indices(transformation.entities.size());
			//		std::iota(std::begin(ordered_indices), std::end(ordered_indices), 0);

			//		auto zip = psl::zip(transformation.entities, transformation.indices, ordered_indices);

			//		// unwind existing entities
			//		// todo instead of sort, implement this as a sweeping swap
			//		psl::sorting::quick(std::begin(zip), std::end(zip), [](const auto& lhs, const auto& rhs)
			//			{
			//				return lhs.get<1>() < rhs.get<1>();
			//			});

			//		std::tuple<psl::array<entity_t>, psl::array<entity_t>, psl::array<entity_t>> diff_set{};

			//		// apply the normal merging operations, keeping track of index changes
			//		std::set_difference(std::begin(zip), std::end(zip), end, std::end(result),
			// special_inserter(diff_set),
			//			[](const auto& lhs, const auto& rhs)
			//			{
			//				if constexpr (std::is_same_v<decltype(lhs), const entity_t&>)
			//					return rhs.get<0>() < lhs;
			//				else
			//					return lhs.get<0>() < rhs; });

			//		transformation.entities = std::move(std::get<0>(diff_set));
			//		transformation.entities.insert(std::end(transformation.entities), begin, end);

			//		transformation.indices.resize(transformation.entities.size());
			//		std::iota(std::begin(transformation.indices), std::end(transformation.indices), 0);

			//		ordered_indices = std::move(std::get<2>(diff_set));
			//		auto size = ordered_indices.size();
			//		ordered_indices.resize(ordered_indices.size() + std::distance(begin, end));
			//		std::fill(std::next(std::begin(ordered_indices), size), std::next(std::begin(ordered_indices),
			// std::distance(begin, end)), std::numeric_limits<entity_t::size_type>::max());

			//		zip = psl::zip(transformation.entities, transformation.indices, ordered_indices);
			//		// unwind existing entities
			//		// todo instead of sort, implement this as a sweeping swap
			//		psl::sorting::quick(std::begin(zip), std::end(zip), [](const auto& lhs, const auto& rhs)
			//			{
			//				return lhs.get<2>() < rhs.get<2>();
			//			});


			//		// apply order_by and on_condition storing the index changes
			//	}
			//}
			// else
			{
				// here the following operations happen
				// - we make a difference set between the existing entities (entities) and the new source
				// entities (unfiltered)
				// - we then append the list of filtered source entities to the resulting difference set
				// - as both are already sorted at this point, we can use std::inplace_merge to merge the two
				//
				// If we did not do a difference set with the original source we'd have to run a std::unique on the
				// full entities. This could be cheaper but we'd need to benchmark it or do some napkin math
				// first. todo(jdl): do napkin math. Most likely if the filtered source is smaller than the existing
				// entities it would be worthwhile to do the post-unique instead of the difference set.

				psl::array<entity_t> source_cpy;
				if(!std::is_sorted(std::begin(source), std::end(source))) {
					source_cpy = psl::array<entity_t>(source.begin(), source.end());
					std::sort(std::begin(source_cpy), std::end(source_cpy));
					source = psl::array_view<entity_t>(source_cpy.data(), source_cpy.size());
				}

				if(!entities.empty()) {
					psl::array<entity_t> diff_set {};
					std::set_difference(std::begin(entities),
										std::end(entities),
										std::begin(source),
										std::end(source),
										std::back_inserter(diff_set));
					entities = std::move(diff_set);
				}

				auto size = std::size(entities);
				entities.insert(std::end(entities), begin, end);


				for(auto& transformation : transformations) {
					if(!transformation.group || !transformation.group->order_by ||
					   !transformation.group->on_condition.empty() ||
					   (group->relationship & entity_relationship::self) != group->relationship) {
						continue;
					}

					if(transformation.entities.size() != size) {
						transformation.entities = psl::array<entity_t>(entities.begin(), entities.begin() + size);
					}
					transformation.entities.insert(std::end(transformation.entities), begin, end);
					transformation.group->transform(
					  std::begin(transformation.entities), std::end(transformation.entities), state);
					transformation.should_generate = false;
				}

				std::inplace_merge(std::begin(entities), std::next(std::begin(entities), size), std::end(entities));
			}
		}
	}

	// if we have more relationships to resolve other than ourselves we need to resolve those now.
	if((group->relationship & entity_relationship::self) != group->relationship) {
		auto relationship_entities =
		  get_all_relationships_unfiltered(state, psl::array_view<entity_t> {entities}, group->relationship);
		direct_entities = entities;
		std::sort(std::begin(relationship_entities), std::end(relationship_entities));
		relationship_entities.erase(std::unique(std::begin(relationship_entities), std::end(relationship_entities)),
									std::end(relationship_entities));
		relationship_entities.erase(
		  group->execute(get_component_container, std::begin(relationship_entities), std::end(relationship_entities)),
		  std::end(relationship_entities));

		// if we don't have a self relationship we can just replace the entities with the new list, otherwise
		// we need to merge them.
		if((group->relationship & entity_relationship::self) != entity_relationship::self) {
			entities = relationship_entities;
		} else {
			const auto size = entities.size();
			entities.insert(std::end(entities),
							std::make_move_iterator(std::begin(relationship_entities)),
							std::make_move_iterator(std::end(relationship_entities)));
			std::inplace_merge(std::begin(entities), std::next(std::begin(entities), size), std::end(entities));
			entities.erase(std::unique(std::begin(entities), std::end(entities)), std::end(entities));
		}
	}

	psl_assert(std::unique(std::begin(entities), std::end(entities)) == std::end(entities),
			   "some entities were not unique");
	psl_assert(std::all_of(std::begin(group->on_combine),
						   std::end(group->on_combine),
						   [this, &get_component_container](auto filter) {
							   auto cInfo = get_component_container(filter);
							   return std::all_of(std::begin(entities),
												  std::end(entities),
												  [filter, &cInfo](entity_t e) { return cInfo->has_storage_for(e); });
						   }),
			   "some components failed to have storage for the entities");
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	static constexpr auto erd_key = component_key_t::generate<entity_relationship_data_t>();
	if(std::any_of(std::begin(group->filters), std::end(group->filters), [](auto const& container) {
		   return container.key == erd_key;
	   })) {
		auto component_container = get_component_container(erd_key);
		for(auto e : entities) {
			if(!component_container->has(e)) {
				entity_relationship_data_t value {e};
				component_container->add(e, &value);
			}
		}
	}
#endif
	for(auto& transformation : transformations) {
		if(!transformation.should_generate) {
			continue;
		}

		transformation.entities = entities;
		transformation.entities.erase(transformation.group->transform(
										std::begin(transformation.entities), std::end(transformation.entities), state),
									  std::end(transformation.entities));

		transformation.should_generate = false;
	}
}
}	 // namespace psl::ecs::details
