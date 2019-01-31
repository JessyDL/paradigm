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

		for(auto[velocity, transform] : movables)
		{
			transform.position += velocity.direction * velocity.force * dTime.count();
			transform.rotation = normalize(psl::quat(0.8f* dTime.count(), 0.0f, 0.0f, 1.0f) * transform.rotation);
		}

		return core::ecs::command_buffer{state};
	};
} // namespace core::ecs::systems