#pragma once
#include "ecs/ecs.hpp"
#include "gfx/pass.h"
#include "systems/resource.h"

namespace core::ecs::components
{
	struct transform;
	struct renderable;
} // namespace core::ecs::components

namespace core::ecs::systems
{
	class geometry_instance
	{
		core::ecs::vector<core::ecs::components::transform, core::ecs::READ_WRITE> m_Transforms;
		core::ecs::vector<core::ecs::components::renderable, core::ecs::READ_ONLY> m_Renderers;
		core::ecs::vector<core::ecs::entity> m_Entities;

		core::ecs::vector<core::ecs::components::transform, core::ecs::READ_ONLY> m_CamTransform;
		core::ecs::vector<core::ecs::entity> m_CamEntities;

	public:
		void announce(core::ecs::state& state);
		void tick(core::ecs::state& state, std::chrono::duration<float> dTime);
	private:
	};
}