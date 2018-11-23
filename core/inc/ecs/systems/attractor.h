#pragma once

#include "ecs/ecs.hpp"

namespace core::ecs::components
{
	struct transform;
	struct velocity;

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
	class attractor
	{
		core::ecs::vector<const core::ecs::components::transform> m_Transforms;
		core::ecs::vector<core::ecs::components::velocity> m_Velocity;
		core::ecs::vector<core::ecs::entity> m_Entities;

		core::ecs::vector<const core::ecs::components::transform> m_AttractorTransform;
		core::ecs::vector<const core::ecs::components::attractor> m_Attractors;
		core::ecs::vector<core::ecs::entity> m_AttractorEntities;

	public:
		void announce(core::ecs::state& state);
		void tick(core::ecs::state& state, std::chrono::duration<float> dTime);
	private:
	};
}