#pragma once
#include "math/math.hpp"

namespace core::ecs::components
{
	struct camera
	{
		float fov{60};
		float near{0.1f};
		float far{240.0f};
		core::resource::indirect_handle<core::gfx::pass> pass;
		std::array<uint32_t, 16> layers{0};
	};
}