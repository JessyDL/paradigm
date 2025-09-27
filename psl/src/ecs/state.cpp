
#include "psl/ecs/state.hpp"
#include "psl/algorithm.hpp"
#include "psl/async/async.hpp"
#include "psl/memory/region.hpp"
#include "psl/unique_ptr.hpp"
#include "tracy/Tracy.hpp"

#include "psl/ecs/details/system_handler.hpp"


#include <atomic>
#include <numeric>

#include <tbb/flow_graph.h>
#include <tbb/tbb.h>


using namespace psl::ecs;

using psl::ecs::details::component_key_t;

state_t::state_t(size_t workers, size_t cache_size, entity_t::size_type min_entities_per_worker)
	: details::entity_relationship_handler_t::entity_relationship_handler_t(), entity_container_t::entity_container_t(),
	  details::components_cache_t::components_cache_t(), m_MinEntitiesPerWorker(min_entities_per_worker) {
	m_SystemGroups.emplace(0, psl::array<details::system_token> {});
	m_SystemHandler = std::make_unique<details::system_handler_t>();
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	create_storage<entity_relationship_data_t>();
#endif
}

state_t::~state_t() = default;


void state_t::tick(std::chrono::duration<float> dTime) {
	tick(dTime, psl::array_view<system_group_t> {});
}
void state_t::tick(std::chrono::duration<float> dTime, system_group_t group) {
	tick(dTime, psl::array_view<system_group_t> {&group, 1});
}
void state_t::tick(std::chrono::duration<float> dTime, psl::array_view<system_group_t> groups) {
	ZoneScoped;
	m_LockState = 1;

#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	update_relationship_components(get_component_typed_info<entity_relationship_data_t>());
#endif

	// remove filters that are no longer in use
	m_Filters.erase(std::remove_if(begin(m_Filters),
								   end(m_Filters),
								   [](const details::filter_result& res) { return res.group.use_count() <= 1; }),
					end(m_Filters));

	auto mod_entities			= entity_container_t::modified_entities();
	auto mod_hierarchy_entities = psl::array<entity_t> {modified_hierarchy_entities()};
	std::sort(std::begin(mod_entities), std::end(mod_entities));
	std::sort(std::begin(mod_hierarchy_entities), std::end(mod_hierarchy_entities));

	psl::array<details::component_container_t*> mutated_components = apply_mutations();

	std::vector<details::system_handler_t::system_task_t> systems_to_execute {};
	if(groups.size() == 0) {
		for(auto& [id, system] : m_SystemInformations) {
			if(m_SystemGroupIndices.find(id) != std::end(m_SystemGroupIndices)) {
				continue;
			}
			auto& task	= systems_to_execute.emplace_back();
			task.id		= id;
			task.system = &system;

			for(auto& filter : system.m_Filters) {
				auto filter_it = std::find_if(begin(m_Filters), end(m_Filters), [&filter](const auto& data) {
					return data.group && *data.group == *filter;
				});
				psl_assert(filter_it != std::end(m_Filters),
						   "Could not find a matching filter for the system {} with debug name '{}'",
						   id.value(),
						   system.debug_name());
				task.filters.insert(filter_it->id);
			}
		}
	}

	auto command_buffers = m_SystemHandler->execute(
	  {
		.modified_entities = mod_entities,
		.mutated_entities  = mod_hierarchy_entities,
		.state			   = this,
		.filter_handler	   = this,
	  },
	  systems_to_execute,
	  dTime,
	  dTime,
	  m_Tick);

	clear_modified_entities();
	clear_modified_hierarchy();

	// todo: we can optimize this, and additionally the filters should be refined for the systems that we'll
	// actually use
	//
	// when ticking if the system is a group we go down an alternate pathway where we do not do any component
	// promotion or filtering, but instead we just execute the systems in the group additionally the tick value does
	// not increment. new systems can be added and removed during this tick, but that's the only shared
	// functionality between the two. as group ticks can not use advanced filtering operations they will
	// additionally not see new components until a normal tick is performed.
	auto system_indices = std::unordered_set<details::system_token>();

	process_to_be_orphans();
	components_cache_t::purge();

	// we can clear the mutated component data now as we have the filtering information:
	for(auto* cInfo : mutated_components) {
		if(cInfo) {
			cInfo->clear();
		}
	}

	for(auto& info : command_buffers) {
		execute_command_buffer(info.get()->command_buffer);
	}
	m_InfoBuffer.clear();

	++m_Tick;

	// here we clean up the transient filters (on_add/on_combine) which need to be merged with the pre-existing
	// filters (if available) if they are available then we will look through all the systems and update the
	// filters. if not, they become the new permanent filter.
	auto transient_filters_it =
	  std::stable_partition(std::begin(m_Filters), std::end(m_Filters), [](const details::filter_result& data) {
		  return !data.group->is_transient();
	  });
	for(auto it = transient_filters_it; it != std::end(m_Filters); ++it) {
		if(it->group && it->group->is_transient()) {
			it->group->disable_transience();

			auto found_it =
			  std::find_if(std::begin(m_Filters), transient_filters_it, [&it](const details::filter_result& data) {
				  return *data.group == *it->group;
			  });

			// in case an already existing filter was found we will update it the systems to the pre-existing filter
			if(found_it != std::end(m_Filters)) {
				for(auto& [id, system] : m_SystemInformations) {
					auto system_filter_it = std::find_if(std::begin(system.m_Filters),
														 std::end(system.m_Filters),
														 [&it](const auto& filter) { return *filter == *it->group; });
					if(system_filter_it != std::end(system.m_Filters)) {
						*system_filter_it = found_it->group;
					}
				}
			}
		}
	}
	// run another gc pass to remove filters that are no longer in use.
	m_Filters.erase(std::remove_if(begin(m_Filters),
								   end(m_Filters),
								   [](const details::filter_result& res) { return res.group.use_count() <= 1; }),
					end(m_Filters));

	if(m_NewSystemInformations.size() > 0) {
		for(auto& [id, system] : m_NewSystemInformations) {
			m_SystemInformations.emplace(id, std::move(system));
		}
		m_NewSystemInformations.clear();
	}
	m_LockState = 0;

	if(m_ToRevoke.size() > 0) {
		if(groups.size() == 0) {
			for(auto id : m_ToRevoke) {
				revoke(id);
			}
			m_ToRevoke.clear();
		} else {
			m_ToRevoke.erase(
			  std::remove_if(std::begin(m_ToRevoke), std::end(m_ToRevoke), [&system_indices, this](auto id) {
				  if(system_indices.contains(id)) {
					  revoke(id);
					  return true;
				  }
				  return false;
			  }));
		}
	}
}

// consider an alias feature
// ie: alias transform = position, rotation, scale components
void state_t::destroy(psl::array_view<entity_t> entities) noexcept {
	ZoneScoped;
	if(entities.size() == 0)
		return;

	components_cache_t::destroy_components(entities);
	entity_container_t::destroy(entities);
}

void state_t::destroy(entity_t entity) noexcept {
	ZoneScoped;
	components_cache_t::destroy_components(entity);
	entity_container_t::destroy(entity);
}

void state_t::reset(psl::array_view<entity_t> entities) noexcept {
	ZoneScoped;
	psl::array<entity_t> storage {};
	components_cache_t::destroy_components(entities);
}
psl::array<entity_t>::iterator state_t::filter(details::filter_group const& group,
											   psl::array<entity_t>::iterator begin,
											   psl::array<entity_t>::iterator end) const noexcept {
	return group.execute([this](component_key_t const& key) { return get_component_container(key); }, begin, end);
}

void state_t::initialize(details::filter_result& data) const noexcept {
	data.initialize(
	  *this,
	  [this](component_key_t const& key) { return get_component_container(key); },
	  [this](entity_t e) { return change_event(e); },
	  [this](entity_t e) { return get_relationship(e); });
}

void state_t::filter(details::filter_result& data, psl::array_view<entity_t> source) const noexcept {
	data.execute(
	  source,
	  *this,
	  [this](component_key_t const& key) { return get_component_container(key); },
	  [this](entity_t e) { return change_event(e); },
	  [this](entity_t e) { return get_relationship(e); });
}

void state_t::execute_command_buffer(command_buffer_t& command_buffer) {
	ZoneScoped;

	auto destroyed_entities = command_buffer.m_DestroyedEntities;
	auto mid =
	  std::partition(std::begin(destroyed_entities),
					 std::end(destroyed_entities),
					 [first = command_buffer.m_First](auto e) { return static_cast<entity_t::size_type>(e) >= first; });

	psl::sparse_array<entity_t::size_type, entity_t::size_type> remapped_entities;
	if(command_buffer.m_Entities.size() > 0) {
		psl::array<entity_t> added_entities;
		std::set_difference(command_buffer.m_Entities.data(),
							command_buffer.m_Entities.data() + command_buffer.m_Entities.size(),
							command_buffer.m_DestroyedEntities.data(),
							command_buffer.m_DestroyedEntities.data() + command_buffer.m_DestroyedEntities.size(),
							std::back_inserter(added_entities));

		auto new_entities = create(added_entities.size());
		psl_assert(new_entities.size() == added_entities.size(), "new entities size should match added entities size");
		auto new_entities_it = std::begin(new_entities);
		for(auto e : added_entities) {
			remapped_entities.insert(e.value(), new_entities_it->value());
			++new_entities_it;
		}
		modify_entities(added_entities);
	}

	components_cache_t::execute_command_buffer(command_buffer, remapped_entities);
	auto span = command_buffer.m_ModifiedEntities.indices();
	psl::array_view<entity_t> indices {const_cast<entity_t*>(reinterpret_cast<entity_t const*>(span.data())),
									   span.size()};
	modify_entities(indices);

	if(mid != std::end(destroyed_entities)) {
		destroy(
		  psl::array_view<entity_t> {&*mid, static_cast<size_t>(std::distance(mid, std::end(destroyed_entities)))});
	}
}

void state_t::clear(bool release_memory) noexcept {
	components_cache_t::clear(release_memory);
	entity_relationship_handler_t::clear();
	entity_container_t::clear();

	m_Tick = 0;
	m_SystemInformations.clear();
	m_NewSystemInformations.clear();
	m_Filters.clear();
	m_LockState = 0;
	m_ToRevoke.clear();
	m_SystemGroupCounter = 1;
	m_SystemGroups.clear();
	m_SystemGroupIndices.clear();
	m_SystemGroups.emplace(0, psl::array<details::system_token> {});
#if !defined(PE_ECS_DISABLE_ENTITY_HIERARCHY)
	create_storage<entity_relationship_data_t>();
#endif
}
