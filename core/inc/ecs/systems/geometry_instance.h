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
	struct input_tag;
} // namespace core::ecs::components

namespace core::ecs::systems
{
	class geometry_instance
	{
		core::ecs::pack<const core::ecs::components::renderable,
						const core::ecs::components::velocity, core::ecs::components::transform>
			m_Geometry;

		core::ecs::pack<const core::ecs::components::transform, core::ecs::filter<core::ecs::components::input_tag>>
			m_Cameras;

		core::ecs::entity_pack<core::ecs::components::lifetime> m_Lifetimes;

	  public:
		geometry_instance(core::ecs::state& state);

		void tick(core::ecs::state& state, std::chrono::duration<float> dTime);

	  private:
	};
} // namespace core::ecs::systems