#pragma once
#include "math/math.hpp"
#include "stdafx.h"

namespace core::ecs::components
{
	struct renderable
	{
		UID material;
		UID geometry;
		uint32_t layer;
	};
}