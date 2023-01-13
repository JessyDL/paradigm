#pragma once
#include "ecs/components/dead_tag.hpp"
#include "ecs/components/lifetime.hpp"
#include "psl/ecs/command_buffer.hpp"
#include "psl/ecs/state.hpp"


namespace core::ecs::systems {
auto lifetime = [](psl::ecs::info_t& info,
				   psl::ecs::pack_direct_partial_t<psl::ecs::entity, core::ecs::components::lifetime> life_pack) {
	using namespace psl::ecs;
	using namespace core::ecs::components;

	std::vector<entity> dead_entities;
	for(auto [entity, lifetime] : life_pack) {
		lifetime.value -= info.dTime.count();
		if(lifetime.value <= 0.0f)
			dead_entities.emplace_back(entity);
	}

	if(dead_entities.size() > 0) {
		info.command_buffer.destroy(dead_entities);
		// info.command_buffer.add_components<dead_tag>(dead_entities);
		// info.command_buffer.remove_components<components::lifetime>(dead_entities);
	}
};
}
