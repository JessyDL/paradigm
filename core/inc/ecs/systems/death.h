#pragma once
#include "ecs/state.h"
#include "ecs/components/dead_tag.h"

namespace core::ecs::systems
{
	auto death = []
		(psl::ecs::info& info,
		 psl::ecs::pack<psl::ecs::partial, psl::ecs::entity, psl::ecs::on_add<core::ecs::components::dead_tag>> dead_pack)
	{
		//info.command_buffer.destroy(dead_pack.get<core::ecs::entity>());
	};
}