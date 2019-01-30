#pragma once
#include "ecs/entity.h"
#include "ecs/selectors.h"
#include "ecs/pack.h"
#include "ecs/command_buffer.h"
#include "ecs/components/lifetime.h"
#include "ecs/components/dead_tag.h"


namespace core::ecs::systems
{
	auto lifetime = [](const core::ecs::state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
					   core::ecs::pack<core::ecs::entity, core::ecs::components::lifetime> life_pack)
	{
		using namespace core::ecs;
		using namespace core::ecs::components;

		std::vector<entity> dead_entities;
		for(auto[entity, lifetime] : life_pack)
		{
			lifetime.value -= dTime.count();
			if(lifetime.value <= 0.0f)
				dead_entities.emplace_back(entity);
		}

		core::ecs::command_buffer commands{state};
		commands.add_component<dead_tag>(dead_entities);
		commands.remove_component<components::lifetime>(dead_entities);
		return commands;
	};
}