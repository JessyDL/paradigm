#pragma once
#include "psl/math/math.hpp"

namespace core::ecs::components
{
struct velocity
{
	velocity() = default;
	velocity(psl::vec3 direction, float force, float inertia) : direction(direction), force(force), inertia(inertia) {};
	psl::vec3 direction;
	float force;
	float inertia;
};
}	 // namespace core::ecs::components
