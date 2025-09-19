#pragma once
#include "psl/math/math.hpp"

namespace core::ecs::components {
struct velocity {
	velocity() = default;
	velocity(psl::vec3 direction, float force, float inertia) : direction(direction), force(force), inertia(inertia) {};
	psl::vec3 direction {psl::vec3::up};
	float force {0.f};
	float inertia {0.f};
};
}	 // namespace core::ecs::components
