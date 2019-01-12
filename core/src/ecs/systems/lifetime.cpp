
#include "ecs/systems/lifetime.h"
#include "ecs/components/lifetime.h"
#include "ecs/components/dead_tag.h"

using namespace core::ecs::components;
using namespace core::ecs::systems;
using namespace core::ecs;

systems::lifetime::lifetime(state& state)
{
	state.register_system(*this);
	state.register_dependency(*this, ecs::tick{}, m_Lifetime);
}

void systems::lifetime::tick(commands& commands, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime)
{
	PROFILE_SCOPE(core::profiler)
	std::vector<entity> dead_entities;
	for (auto[entity, lifetime] : m_Lifetime)
	{
		lifetime.value -= dTime.count();
		if (lifetime.value <= 0.0f)
			dead_entities.emplace_back(entity);
	}

	commands.add_component<dead_tag>(dead_entities);
	commands.remove_component<components::lifetime>(dead_entities);
}