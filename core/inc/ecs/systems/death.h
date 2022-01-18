#pragma once
#include "ecs/components/dead_tag.h"
#include "psl/ecs/state.h"

namespace core::ecs::systems
{
	auto death =
	  [](psl::ecs::info_t& info,
		 psl::ecs::pack<psl::ecs::partial, psl::ecs::entity, psl::ecs::on_add<core::ecs::components::dead_tag>>
		   dead_pack) { info.command_buffer.destroy(dead_pack.get<psl::ecs::entity>()); };
}