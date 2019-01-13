
#include "ecs/systems/death.h"
#include "ecs/components/dead_tag.h"

using namespace core::ecs::components;
using namespace core::ecs::systems;
using namespace core::ecs;


template <typename T>
struct func_traits : public func_traits<decltype(&T::operator())>
{};

template <typename C, typename Ret, typename... Args>
struct func_traits<Ret (C::*)(Args...)>
{
	using result_t	= Ret;
	using arguments_t = std::tuple<Args...>;
};

template <typename Ret, typename... Args>
struct func_traits<Ret (*)(Args...)>
{
	using result_t	= Ret;
	using arguments_t = std::tuple<Args...>;
};

template <typename C, typename Ret, typename... Args>
struct func_traits<Ret (C::*)(Args...) const>
{
	using result_t	= Ret;
	using arguments_t = std::tuple<Args...>;
};

template <class Fn, class... Args>
void test_func(Fn&& fn, Args&&... args)
{
	if constexpr(std::is_member_function_pointer<Fn>::value)
	{

	}
	//static_assert(std::is_pointer<typename std::decay<Fn>::type>::value);
	//static_assert(std::is_member_function_pointer<Fn>::value);
	auto dSys = std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...);
	auto y	= [&dSys]() {
		   auto tuple = typename func_traits<typename std::decay<Fn>::type>::arguments_t{2, 2.0};
		   dSys.operator()(std::get<0>(tuple), std::get<1>(tuple));
	};
}

template <class Fn>
void test_func(Fn&& fn)
{
	auto y = [&fn]() {
		auto tuple = typename func_traits<typename std::decay<Fn>::type>::arguments_t{3, 2.0};
		fn.operator()(std::get<0>(tuple), std::get<1>(tuple));
	};
}
void test(int, double) {}

death::death(state& state)
{
	state.register_system(*this);
	state.register_dependency(*this, ecs::tick{}, m_Dead);

	test_func(test, std::placeholders::_1, std::placeholders::_2);
	test_func(&death::easy, this, std::placeholders::_1, std::placeholders::_2);
	test_func([](int x, double y) { x = 0; });

	auto dSys = std::bind(&death::easy, this, std::placeholders::_1, std::placeholders::_2);
	auto y	= [&dSys]() { dSys.operator()(5, 3.0); };
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