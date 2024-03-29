#pragma once
#include "core/ecs/components/dead_tag.hpp"
#include "psl/ecs/state.hpp"

namespace core::ecs::systems {
auto death =
  [](psl::ecs::info_t& info,
	 psl::ecs::pack_direct_partial_t<psl::ecs::entity_t, psl::ecs::on_add<core::ecs::components::dead_tag>> dead_pack) {
	  info.command_buffer.destroy(dead_pack.get<psl::ecs::entity_t>());
  };
}
