
#include "ecs/systems/death.h"
#include "ecs/components/dead_tag.h"

using namespace core::ecs::components;
using namespace core::ecs::systems;
using namespace core::ecs;


void test(int, double) {}

death::death(state& state)
{
	state.register_system(*this);
	state.register_dependency(*this, ecs::tick{}, m_Dead);

	//state.declare(test, std::placeholders::_1, std::placeholders::_2);
	state.declare(&death::death_system, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	//state.declare([](int x, double y) { x = 0; });
}

void death::easy(int, double) {}
void death::tick(commands& commands, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime)
{
	PROFILE_SCOPE(core::profiler)
	commands.destroy(m_Dead.get<core::ecs::entity>());
}

void death::death_system(
	commands& commands, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
	core::ecs::pack<core::ecs::entity, core::ecs::on_add<core::ecs::components::dead_tag>> dead_pack)
{
	PROFILE_SCOPE(core::profiler)
	commands.destroy(dead_pack.get<core::ecs::entity>());
}