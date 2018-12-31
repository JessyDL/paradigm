#include "stdafx.h"
#include "ecs/systems/movement.h"
#include "ecs/components/velocity.h"
#include "ecs/components/transform.h"

using namespace psl::math;
using namespace core::ecs;
using namespace core::ecs::systems;
using namespace core::ecs::components;

movement::movement(state& state)
{
	state.register_system(*this);
	state.register_dependency(*this, ecs::tick{}, m_Movable);
}

void movement::tick(core::ecs::state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime)
{
	PROFILE_SCOPE(core::profiler)
	for (auto [velocity, transform] : m_Movable)
	{
		transform.position += velocity.direction * velocity.force * dTime.count();
		transform.rotation = normalize(psl::quat(0.8f* dTime.count(), 0.0f, 0.0f, 1.0f) * transform.rotation);
	}
}