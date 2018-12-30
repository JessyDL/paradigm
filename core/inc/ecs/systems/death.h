#pragma once
#include "ecs/ecs.hpp"

namespace core::ecs::components
{
	struct lifetime;
	struct dead_tag;
}


namespace core::ecs::systems
{
	class death
	{
	public:
		death(core::ecs::state& state);

		void tick(core::ecs::state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime);
	private:

		core::ecs::pack<const core::ecs::components::dead_tag, core::ecs::entity, core::ecs::on_add<core::ecs::components::dead_tag>> m_Dead;
	};
}