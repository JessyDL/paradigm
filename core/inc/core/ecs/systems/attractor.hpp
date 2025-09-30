#pragma once

#include "core/ecs/components/transform.hpp"
#include "core/ecs/components/velocity.hpp"
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
auto attractor = [](psl::ecs::info_t const& info,
					psl::ecs::pack_indirect_partial_t<const core::ecs::components::transform,
													  core::ecs::components::velocity,
													  psl::ecs::filter<core::ecs::components::dynamic_tag>> movables,
					psl::ecs::pack_indirect_full_t<const core::ecs::components::transform,
												   const core::ecs::components::attractor> attractors) {
	using namespace psl::math;
	if(attractors.empty() || movables.empty()) {
		return;
	}

	for(auto [attrTransform, attractor] : attractors) {
		for(auto [movTrans, movVel] : movables) {
			const auto mag =
			  saturate((attractor.radius - magnitude(movTrans.position - attrTransform.position)) / attractor.radius) *
			  info.dTime.count();
			const auto direction = normalize(attrTransform.position - movTrans.position);

			movVel.direction = mix(movVel.direction, direction, mag);
			movVel.force += attractor.force * mag;
		}
	}
};
}	 // namespace core::ecs::systems
