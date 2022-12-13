#pragma once
#include "psl/math/math.hpp"

#undef near
#undef far

namespace core::ivk
{
class drawpass;
}

namespace core::ecs::components
{
struct camera
{
	float fov {60};
	float near {1.0f};
	float far {8192.0f};
	// core::resource::weak_handle<core::ivk::pass> pass;
	std::array<uint32_t, 16> layers {0};
};
}	 // namespace core::ecs::components