#include "stdafx.h"
#include "ecs/systems/attractor.h"
#include "gfx/material.h"
#include "vk/geometry.h"
#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"
#include "ecs/components/input_tag.h"

using namespace core::ecs::systems;
using namespace psl::math;

attractor::attractor(core::ecs::state& state)
{
	state.register_system(*this);
	state.register_dependency(*this, core::ecs::tick{}, m_Movables);
	state.register_dependency(*this, core::ecs::tick{}, m_Attractors);
}

void attractor::tick(core::ecs::state& state, std::chrono::duration<float> dTime)
{
	for (auto [movTrans, movVel] : m_Movables)
	{
		for (auto [attrTransform, attractor] : m_Attractors)
		{
			const auto mag = saturate((attractor.radius - magnitude(movTrans.position - attrTransform.position)) / attractor.radius) * dTime.count();
			const auto direction = normalize(attrTransform.position - movTrans.position) * attractor.force;

			movVel.direction[0] = mix(movVel.direction[0], direction[0], mag);
			movVel.direction[1] = mix(movVel.direction[1], direction[1], mag);
			movVel.direction[2] = mix(movVel.direction[2], direction[2], mag);
		}
	}
}