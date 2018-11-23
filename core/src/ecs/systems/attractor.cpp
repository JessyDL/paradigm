#include "stdafx.h"
#include "ecs/systems/attractor.h"
#include "gfx/material.h"
#include "vk/geometry.h"
#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"
#include "ecs/components/input_tag.h"

using namespace core::ecs::systems;
using namespace psl::math;

void attractor::announce(core::ecs::state& state)
{
	state.register_dependency(*this, {m_Entities, m_Transforms, m_Velocity});
	state.register_dependency(*this, {m_AttractorEntities, m_AttractorTransform, m_Attractors});
}
void attractor::tick(core::ecs::state& state, std::chrono::duration<float> dTime)
{
	for(size_t i = 0; i < m_Entities.size(); ++i)
	{
		for(auto u = 0; u < m_AttractorEntities.size(); ++u)
		{
			const auto mag = saturate((m_Attractors[u].radius - magnitude(m_Transforms[i].position - m_AttractorTransform[u].position)) / m_Attractors[u].radius) * dTime.count();
			const auto direction = normalize(m_AttractorTransform[u].position - m_Transforms[i].position) * m_Attractors[u].force;
			//m_Transforms[i].position[0] = mix(m_Transforms[i].position[0], m_CamTransform[0].position[0], mag);
			//m_Transforms[i].position[1] = mix(m_Transforms[i].position[1], m_CamTransform[0].position[1], mag);
			//m_Transforms[i].position[2] = mix(m_Transforms[i].position[2], m_CamTransform[0].position[2], mag);

			m_Velocity[i].direction[0] = mix(m_Velocity[i].direction[0], direction[0], mag);
			m_Velocity[i].direction[1] = mix(m_Velocity[i].direction[1], direction[1], mag);
			m_Velocity[i].direction[2] = mix(m_Velocity[i].direction[2], direction[2], mag);
			//m_Velocity[i].direction = normalize(m_Velocity[i].direction);
		}
	}
}