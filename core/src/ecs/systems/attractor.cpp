
#include "ecs/systems/attractor.h"
#include "gfx/material.h"
#include "vk/geometry.h"
#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"
#include "ecs/components/input_tag.h"
#include "ecs/components/velocity.h"

using namespace core::ecs::systems;
using namespace psl::math;


void attr_tick(core::ecs::commands& commands, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime, core::ecs::pack<const core::ecs::components::transform,
	core::ecs::components::velocity> movables, core::ecs::pack<const core::ecs::components::transform,
			   const core::ecs::components::attractor> attractors)
{
	for(auto[movTrans, movVel] : movables)
	{
		for(auto[attrTransform, attractor] : attractors)
		{
			const auto mag = saturate((attractor.radius - magnitude(movTrans.position - attrTransform.position)) / attractor.radius) * dTime.count();
			const auto direction = normalize(attrTransform.position - movTrans.position) * attractor.force;

			movVel.direction = mix<float, float, float, 3>(movVel.direction, direction, mag);
		}
	}
}
attractor::attractor(core::ecs::state& state)
{
	state.declare(attr_tick);
	//state.register_system(*this);
	//state.register_dependency(*this, core::ecs::tick{}, m_Movables);
	//state.register_dependency(*this, core::ecs::tick{}, m_Attractors);
}

void attractor::tick(core::ecs::commands& commands, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime)
{
	PROFILE_SCOPE(core::profiler)

	for (auto [movTrans, movVel] : m_Movables)
	{
		for (auto [attrTransform, attractor] : m_Attractors)
		{
			const auto mag = saturate((attractor.radius - magnitude(movTrans.position - attrTransform.position)) / attractor.radius) * dTime.count();
			const auto direction = normalize(attrTransform.position - movTrans.position) * attractor.force;

			movVel.direction = mix<float, float, float, 3>(movVel.direction, direction, mag);
		}
	}
}