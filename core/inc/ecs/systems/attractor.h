#pragma once

#include "ecs/command_buffer.h"
#include "ecs/pack.h"
#include "ecs/components/velocity.h"
#include "ecs/components/transform.h"
#include "math/math.hpp"
#include <chrono>

namespace core::ecs::components
{
	struct attractor
	{
		attractor() = default;
		attractor(float force, float radius) : force(force), radius(radius){};
		float force;
		float radius;
	};
} // namespace core::ecs::components

namespace core::ecs::systems
{
	auto attractor =
		[](const core::ecs::state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
		   core::ecs::pack<core::ecs::partial, const core::ecs::components::transform, core::ecs::components::velocity>
			   movables,
		   core::ecs::pack<core::ecs::full, const core::ecs::components::transform,
						   const core::ecs::components::attractor>
			   attractors) {
			using namespace psl::math;

			for(auto [movTrans, movVel] : movables)
			{
				for(auto [attrTransform, attractor] : attractors)
				{
					const auto mag =
						saturate((attractor.radius - magnitude(movTrans.position - attrTransform.position)) /
								 attractor.radius) *
						dTime.count();
					const auto direction = normalize(attrTransform.position - movTrans.position) * attractor.force;

					movVel.direction = mix<float, float, float, 3>(movVel.direction, direction, mag);
				}
			}

			return core::ecs::command_buffer{state};
		};
}