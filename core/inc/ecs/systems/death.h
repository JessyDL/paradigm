#pragma once
#include "ecs/ecs.hpp"
#include "ecs/components/dead_tag.h"

namespace core::ecs::systems
{
	auto death = []
		(const core::ecs::state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
		 core::ecs::pack<core::ecs::partial, core::ecs::entity, core::ecs::on_add<core::ecs::components::dead_tag>> dead_pack)
	{
		core::ecs::command_buffer commands{state};
		commands.destroy(dead_pack.get<core::ecs::entity>());
		return commands;
	};
}