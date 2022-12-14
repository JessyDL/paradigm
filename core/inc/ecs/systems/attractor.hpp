#pragma once

#include "ecs/components/transform.hpp"
#include "ecs/components/velocity.hpp"
#include "psl/ecs/state.hpp"
#include "psl/math/math.hpp"
#include <chrono>

namespace core::ecs::components {
struct attractor {
	attractor() = default;
	attractor(float force, float radius) : force(force), radius(radius) {};
	float force;
	float radius;
};
}	 // namespace core::ecs::components

namespace core::ecs::systems {
auto attractor =
  [](psl::ecs::info_t& info,
	 psl::ecs::pack<psl::ecs::partial,
					const core::ecs::components::transform,
					core::ecs::components::velocity,
					psl::ecs::filter<core::ecs::components::dynamic_tag>> movables,
	 psl::ecs::pack<psl::ecs::full, const core::ecs::components::transform, const core::ecs::components::attractor>
	   attractors) {
	  using namespace psl::math;

	  for(auto [movTrans, movVel] : movables) {
		  for(auto [attrTransform, attractor] : attractors) {
			  const auto mag = saturate((attractor.radius - magnitude(movTrans.position - attrTransform.position)) /
										attractor.radius) *
							   info.dTime.count();
			  const auto direction = normalize(attrTransform.position - movTrans.position) * attractor.force;

			  movVel.direction = mix(movVel.direction, direction, mag);
		  }
	  }
  };
}
