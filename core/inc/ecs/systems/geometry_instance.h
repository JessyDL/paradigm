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
		core::ecs::pack<const core::ecs::components::renderable, core::ecs::components::transform>
			m_Geometry;

		
	  public:
		geometry_instance(core::ecs::state& state);

		void tick(core::ecs::commands& commands, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime);

	  private:
	};
} // namespace core::ecs::systems