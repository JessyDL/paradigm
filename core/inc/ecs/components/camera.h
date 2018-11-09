#pragma once
#include "math/math.hpp"

namespace core::ecs::components
{
	struct camera
	{
		float fov;
		float near;
		float far;
		std::array<uint32_t, 256> layers;
	};
}