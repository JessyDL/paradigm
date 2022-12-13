#pragma once
#include "psl/math/vec.hpp"

namespace core::ecs::components
{
struct light
{
	struct directional
	{};

	struct point
	{
		float radius;
	};

	struct spot
	{
		float innerAngle;
		float outerAngle;
		float length;
	};
	enum class type : uint8_t
	{
		DIRECTIONAL = 0,
		POINT		= 1,
		SPOT		= 2,
		AREA		= 3
	};

	light() = default;
	light(psl::vec3 color, float intensity, type type, bool shadows) :
		color(color), intensity(intensity), type(type), shadows(shadows) {};

	psl::vec3 color;
	float intensity;
	type type;
	bool shadows;
	union
	{
		light::directional uDirection;
		light::point uPoint;
		light::spot uSpot;
	};
};
}	 // namespace core::ecs::components
