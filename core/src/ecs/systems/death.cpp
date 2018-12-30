#include "stdafx.h"
#include "ecs/systems/death.h"
#include "ecs/components/transform.h"

using namespace core::ecs::components;
using namespace core::ecs::systems;
using namespace core::ecs;

death::death(state& state)
{
	state.register_system(*this);
	state.register_dependency(*this, ecs::tick{}, m_Dead);
}

void death::tick(state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime)
{

	state.destroy(m_Dead.get<core::ecs::entity>());
}