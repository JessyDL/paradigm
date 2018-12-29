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
		core::ecs::pack<const core::ecs::components::transform,
			core::ecs::components::velocity> m_Movables;

		core::ecs::pack<const core::ecs::components::transform,
			const core::ecs::components::attractor> m_Attractors;

	public:
		attractor(core::ecs::state& state);
		void tick(core::ecs::state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime);
	private:
	};
}