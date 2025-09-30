#include "psl/ecs/details/components_cache.hpp"

#include "tracy/Tracy.hpp"

namespace psl::ecs::details {

components_cache_t::components_cache_t() {
#if !defined(PE_ECS_DISABLE_LOOKUP_CACHE)
	static std::mutex mut {};
	static std::atomic<size_t> generation {1};
	std::lock_guard l {mut};
	m_StateUniqueKey = ++generation;
#endif
}

void components_cache_t::execute_command_buffer(
  command_buffer_t& command_buffer,
  psl::sparse_array<entity_t::size_type, entity_t::size_type> const& remapped_entities) {
	ZoneScoped;
	for(auto& component_src : command_buffer.m_Components) {
		if(component_src->entities(true).size() == 0)
			continue;
		auto const key = component_src->id();
		// In the case this is a mutation instruction, we need to remap the component id it uses
		// internally to the target component id. This is a bit messy, but avoids having to recreate
		// the component container.
		if(auto it = command_buffer.m_MutatedComponents.find(key); it != std::end(command_buffer.m_MutatedComponents)) {
			component_src->m_Info.id = it->second;
		}

		auto component_dst = get_component_container(key);

		component_src->remap(remapped_entities, [first = command_buffer.m_First](entity_t e) -> bool {
			return static_cast<entity_t::size_type>(e) >= first;
		});
		if(component_dst == nullptr) {
			m_Components[key] = std::move(component_src);
		} else {
			component_dst->merge(*component_src);
		}
	}

	for(auto& [key, removed_components] : command_buffer.m_RemovedComponents) {
		psl::array<entity_t> ids {};
		ids.reserve(removed_components.size());
		auto component_dst = get_component_container(key);
		psl_assert(component_dst);

		for(auto entity : removed_components) {
			if(entity >= command_buffer.m_First) {
				ids.emplace_back(details::make_entity(remapped_entities[entity]));
			} else {
				ids.emplace_back(details::make_entity(entity));
			}
		}

		component_dst->destroy(ids);
	}
}

void components_cache_t::purge() noexcept {
	ZoneScoped;
	for(auto& [key, cInfo] : m_Components) {
		cInfo->purge();
	}
}


psl::array<details::component_container_t*> components_cache_t::apply_mutations() {
	ZoneScoped;
	psl::array<details::component_container_t*> mutated_components;
	for(auto& [key, cInfo] : m_Components) {
		if(!cInfo || cInfo->size(true) == 0) {
			continue;
		}
		if(cInfo->id() != key) {
			// regardless if the mutation is applied or not, it will be cleared after this operation.
			// mutations do not persist the frame as a design decision.
			mutated_components.push_back(cInfo.get());
			auto targetCInfo = get_component_container(cInfo->id());
			if(!targetCInfo) {
				continue;
			}
			targetCInfo->apply_mutation(cInfo.get());
		}
	}
	return mutated_components;
}

// empty construction
void components_cache_t::add_component_impl(details::component_container_t* cInfo, psl::array_view<entity_t> entities) {
	ZoneScopedN("components_cache_t::add_component (empty)");
	psl_assert(cInfo != nullptr, "component info for key {} was not found", cInfo->id());

	cInfo->add(entities);

#if defined(PE_ECS_FEATURE_COMPONENT_BITSET)
	auto const componentFlag = m_ComponentFlags[cInfo->id()];
	std::bitset<256> entityFlags {};
	entityFlags.set(componentFlag);
	for(auto e : entities) {
		m_EntityFlags[e.value()] |= entityFlags;
	}
#endif
}

void components_cache_t::add_component_impl(const details::component_key_t& key, psl::array_view<entity_t> entities) {
	auto cInfo = get_component_container(key);
	add_component_impl(cInfo, entities);
}

// prototype based construction
void components_cache_t::add_component_impl(details::component_container_t* cInfo,
											psl::array_view<entity_t> entities,
											void* prototype,
											bool repeat) {
	ZoneScopedN("components_cache_t::add_component (prototype)");
	psl_assert(cInfo != nullptr, "component info for key {} was not found", cInfo->id());
	const auto component_size = cInfo->component_type_info().size;
	psl_assert(component_size != 0, "component size was 0");

	cInfo->add(entities, prototype, repeat);

#if defined(PE_ECS_FEATURE_COMPONENT_BITSET)
	auto const componentFlag = m_ComponentFlags[cInfo->id()];
	std::bitset<256> entityFlags {};
	entityFlags.set(componentFlag);
	for(auto e : entities) {
		m_EntityFlags[e.value()] |= entityFlags;
	}
#endif
}
void components_cache_t::add_component_impl(const details::component_key_t& key,
											psl::array_view<entity_t> entities,
											void* prototype,
											bool repeat) {
	auto cInfo = get_component_container(key);
	add_component_impl(cInfo, entities, prototype, repeat);

#if defined(PE_ECS_FEATURE_COMPONENT_BITSET)
	auto const componentFlag = m_ComponentFlags[key];
	std::bitset<256> entityFlags {};
	entityFlags.set(componentFlag);
	for(auto e : entities) {
		m_EntityFlags[e.value()] |= entityFlags;
	}
#endif
}


void components_cache_t::add_component_impl(details::component_container_t* cInfo,
											psl::array_view<entity_t> entities,
											std::function<void()> invocable) {
	ZoneScoped;
	const auto component_size = cInfo->component_type_info().size;
	auto offset				  = cInfo->entities().size();
	cInfo->add(entities);
	std::invoke(invocable);
}

void components_cache_t::remove_component(details::component_container_t* cInfo,
										  psl::array_view<entity_t> entities) noexcept {
	ZoneScoped;
	psl_assert(cInfo != nullptr, "component info for key {} was not found", cInfo->id());
	cInfo->destroy(entities);

#if defined(PE_ECS_FEATURE_COMPONENT_BITSET)
	auto const componentFlag = m_ComponentFlags[cInfo->id()];
	std::bitset<256> entityFlags {};
	entityFlags.set(componentFlag);
	for(auto e : entities) {
		m_EntityFlags[e.value()] &= ~entityFlags;
	}
#endif
}
void components_cache_t::remove_component(const details::component_key_t& key,
										  psl::array_view<entity_t> entities) noexcept {
	auto cInfo = get_component_container(key);
	remove_component(cInfo, entities);
}
void components_cache_t::destroy_components(psl::array_view<entity_t> entities) noexcept {
	ZoneScoped;
	// todo, iterating over the components is expensive
	for(auto& [key, cInfo] : m_Components) {
#if defined(PE_ECS_FEATURE_COMPONENT_BITSET)
		auto const componentFlag = m_ComponentFlags[key];
		if(std::none_of(entities.begin(), entities.end(), [componentFlag, this](entity_t e) {
			   return !m_EntityFlags[e.value()].test(componentFlag);
		   })) {
			continue;
		}
		std::bitset<256> entityFlags {};
		entityFlags.set(componentFlag);
		for(auto e : entities) {
			m_EntityFlags[e.value()] &= ~entityFlags;
		}
#endif

		cInfo->destroy(entities);
	}
}
void components_cache_t::destroy_components(entity_t entity) noexcept {
	ZoneScoped;
	// todo, iterating over thde components is expensive
	for(auto& [key, cInfo] : m_Components) {
#if defined(PE_ECS_FEATURE_COMPONENT_BITSET)
		auto const componentFlag = m_ComponentFlags[key];
		if(!m_EntityFlags[entity.value()].test(componentFlag)) {
			continue;
		}

		std::bitset<256> entityFlags {};
		entityFlags.set(componentFlag);
		m_EntityFlags[entity.value()] &= ~entityFlags;
#endif
		cInfo->destroy(entity);
	}
}

size_t components_cache_t::component_copy_from(psl::array_view<entity_t> entities,
											   const details::component_key_t& key,
											   void* data) noexcept {
	if(entities.size() == 0) {
		return 0;
	}
	ZoneScoped;
	const auto& cInfo = get_component_container(key);
	psl_assert(cInfo != nullptr, "component info for key {} was not found", key);
	return cInfo->copy_from(entities, data);
}

void components_cache_t::clear(bool release_memory) {
	ZoneScoped;
	if(release_memory) {
		m_Components = decltype(m_Components) {};
	} else {
		for(auto& [key, storage] : m_Components) {
			storage->clear();
		}
	}

#if defined(PE_ECS_FEATURE_COMPONENT_BITSET)
	m_NextComponentFlagIndex = 0;
	m_NextComponentFlag		 = {1};
	m_ComponentLookupArray	 = {};
	m_ComponentLookup.clear();
	m_ComponentFlags.clear();
	m_EntityFlags.clear();
#endif
#if !defined(PE_ECS_DISABLE_LOOKUP_CACHE)
	++m_ComponentGeneration;
#endif
}
}	 // namespace psl::ecs::details
