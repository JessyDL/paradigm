#pragma once
#include "ecs/components/transform.hpp"
#include "ecs/components/velocity.hpp"
#include "psl/ecs/state.hpp"

namespace core::ecs::systems {
auto movement =
  [](psl::ecs::info_t& info,
	 psl::ecs::pack<psl::ecs::partial, const core::ecs::components::velocity, core::ecs::components::transform>
	   movables) {
	  using namespace psl::math;
	  using namespace core::ecs;
	  using namespace core::ecs::components;

	  for(auto [velocity, transform] : movables) {
		  transform.position += velocity.direction * velocity.force * info.dTime.count();
		  transform.rotation = normalize(psl::quat(0.8f * info.dTime.count(), 0.0f, 0.0f, 1.0f) * transform.rotation);
	  }
  };
}	 // namespace core::ecs::systems
