#pragma once
#include "ecs/ecs.hpp"

namespace core::ecs::components
{
	struct lifetime;
}


namespace core::ecs::systems
{
	class lifetime
	{
	public:
		lifetime(core::ecs::state& state);

		void tick(core::ecs::state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime);

	private:
		core::ecs::pack<core::ecs::entity, core::ecs::components::lifetime> m_Lifetime;
	};
}