#pragma once
#include "ecs/ecs.hpp"

namespace core::ecs::components
{
	struct transform;
	struct velocity;
} // namespace core::ecs::components

namespace core::ecs::systems
{
	class movement
	{
		core::ecs::pack<const core::ecs::components::velocity, core::ecs::components::transform>
			m_Movable;
	public:
		movement(core::ecs::state& state);

		void tick(core::ecs::commands& commands, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime);

	private:
	};
} // namespace core::ecs::systems