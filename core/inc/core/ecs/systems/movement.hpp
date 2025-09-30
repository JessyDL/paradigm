#pragma once
#include "core/ecs/components/transform.hpp"
#include "core/ecs/components/velocity.hpp"
#include "psl/ecs/state.hpp"

namespace core::ecs::systems {
auto movement =
  [](psl::ecs::info_t const& info,
	 psl::ecs::pack_indirect_partial_t<core::ecs::components::velocity, core::ecs::components::transform> movables) {
	  using namespace psl::math;
	  using namespace core::ecs;
	  using namespace core::ecs::components;

	  for(auto [velocity, transform] : movables) {
		  transform.position += velocity.direction * velocity.force * info.dTime.count();

		  velocity.force *= (1.0f - (velocity.inertia * info.dTime.count()));
		  if(velocity.force < 0.01f) {
			  velocity.force = 0.0f;
		  }
	  }
  };
}	 // namespace core::ecs::systems
