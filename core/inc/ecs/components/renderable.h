#pragma once
#include "math/math.hpp"
#include "stdafx.h"

namespace core::ecs::components
{
	struct renderable
	{
		renderable() = default;
		renderable(const UID& material, const UID& geometry, uint32_t layer) noexcept : material(material), geometry(geometry), layer(layer) {};
		UID material;
		UID geometry;
		uint32_t layer;
	};
}