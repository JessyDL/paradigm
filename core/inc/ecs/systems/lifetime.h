#pragma once
#include "psl/ecs/state.h"
#include "psl/ecs/command_buffer.h"
#include "ecs/components/lifetime.h"
#include "ecs/components/dead_tag.h"


namespace core::ecs::systems
{
	auto lifetime = [](psl::ecs::info& info,
					   psl::ecs::pack<psl::ecs::partial, psl::ecs::entity, core::ecs::components::lifetime> life_pack)
	{
		using namespace psl::ecs;
		using namespace core::ecs::components;

		std::vector<entity> dead_entities;
		for(auto[entity, lifetime] : life_pack)
		{
			lifetime.value -= info.dTime.count();
			if(lifetime.value <= 0.0f)
				dead_entities.emplace_back(entity);
		}

		info.command_buffer.add_components<dead_tag>(dead_entities);
		info.command_buffer.remove_components<components::lifetime>(dead_entities);
	};
}