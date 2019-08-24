#pragma once
#include "psl/math/math.hpp"

#undef near
#undef far

namespace core::ivk
{
	class pass;
}

namespace core::ecs::components
{
	struct camera
	{
		float fov{60};
		float near{0.1f};
		float far{240.0f};
		//core::resource::weak_handle<core::ivk::pass> pass;
		std::array<uint32_t, 16> layers{0};
	};
} // namespace core::ecs::components