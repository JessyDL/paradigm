#pragma once
#include "ecs/ecs.hpp"

namespace core::ecs::components
{
	struct dead_tag;
}


namespace core::ecs::systems
{
	class death
	{
	public:
		death(core::ecs::state& state);

		void tick(core::ecs::commands& commands, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime);
	private:
		void death_system(core::ecs::commands& commands, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime, 
				  core::ecs::pack<core::ecs::entity, core::ecs::on_add<core::ecs::components::dead_tag>> dead_pack);

		void easy(int, double);
		core::ecs::pack<core::ecs::entity, core::ecs::on_add<core::ecs::components::dead_tag>> m_Dead;
	};
}