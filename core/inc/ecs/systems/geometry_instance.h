#pragma once
#include "ecs/ecs.hpp"
#include "gfx/pass.h"
#include "systems/resource.h"

namespace core::ecs::components
{
	struct transform;
	struct renderable;
	struct lifetime;
	struct velocity;
} // namespace core::ecs::components

namespace core::ecs::systems
{
	class geometry_instance
	{
		core::ecs::vector<core::ecs::components::transform> m_Transforms;
		core::ecs::vector<const core::ecs::components::renderable> m_Renderers;
		core::ecs::vector<const core::ecs::components::velocity> m_Velocity;
		core::ecs::vector<core::ecs::entity> m_Entities;

		core::ecs::vector<const core::ecs::components::transform> m_CamTransform;
		core::ecs::vector<core::ecs::entity> m_CamEntities;

		core::ecs::vector<core::ecs::components::lifetime> m_Lifetime;
		core::ecs::vector<core::ecs::entity> m_LifeEntities;
	public:
		void announce(core::ecs::state& state);
		void tick(core::ecs::state& state, std::chrono::duration<float> dTime);
	private:
	};
}