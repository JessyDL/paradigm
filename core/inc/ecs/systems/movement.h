#pragma once
#include "ecs/ecs.hpp"
#include "ecs/components/velocity.h"
#include "ecs/components/transform.h"

namespace core::ecs::systems
{
	auto movement = [](const core::ecs::state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
					   core::ecs::pack<core::ecs::partial, const core::ecs::components::velocity, core::ecs::components::transform> movables)
	{
		using namespace psl::math;
		using namespace core::ecs;
		using namespace core::ecs::components;

		auto begin = movables.begin();
		auto end = movables.end();

		auto velocities = movables.get<const velocity>();
		auto transforms = movables.get<transform>();
		for(size_t i = 0; i < velocities.size(); ++i)
		{
			if((void*)&begin.get<const core::ecs::components::velocity>() > (void*)&velocities[i])
				debug_break();
			if((void*)&velocities[i] >= (void*)&end.get<const core::ecs::components::velocity>())
				debug_break();

			transforms[i].position += velocities[i].direction * velocities[i].force * dTime.count();
			transforms[i].rotation = normalize(psl::quat(0.8f* dTime.count(), 0.0f, 0.0f, 1.0f) * transforms[i].rotation);
		}

		/*for(auto[velocity, transform] : movables)
		{
			if((void*)&begin.get<const core::ecs::components::velocity>() > (void*)&velocity)
				debug_break();
			if((void*)&velocity >= (void*)&end.get<const core::ecs::components::velocity>())
				debug_break();
			transform.position += velocity.direction * velocity.force * dTime.count();
			transform.rotation = normalize(psl::quat(0.8f* dTime.count(), 0.0f, 0.0f, 1.0f) * transform.rotation);
		}*/

		return core::ecs::command_buffer{state};
	};
} // namespace core::ecs::systems