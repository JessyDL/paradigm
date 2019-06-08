#pragma once
#include "math/vec.h"

namespace core::ecs::components
{
	struct directional_shadow_caster_t
	{};

	struct direction_light
	{
		psl::vec4 color{1};
	};
}