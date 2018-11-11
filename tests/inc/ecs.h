#pragma once
#include "ecs/ecs.hpp"

namespace tests::ecs
{
	struct float_system
	{
		core::ecs::vector<float, core::ecs::access::READ_ONLY> m_Floats;
		core::ecs::vector<int, core::ecs::access::READ_WRITE> m_Ints;
		core::ecs::vector<core::ecs::entity> m_Entities;

		void announce(core::ecs::state& state)
		{
			state.register_dependency(*this, {m_Entities, m_Floats, m_Ints});
		}

		void tick(core::ecs::state& state, std::chrono::duration<float> dTime)
		{
			core::log->info("running float_system");
			for (size_t i = 0; i < m_Entities.size(); ++i)
			{
				//core::log->info("    entity - {0} has health {1} and lives {2}", entities[i].id(), m_Floats[i], m_Ints[i]);
				m_Ints[i] += 5;
			}
		}
	};
}