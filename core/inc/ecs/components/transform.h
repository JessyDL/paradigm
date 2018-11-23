#pragma once
#include "math/math.hpp"

namespace core::ecs::components
{
	struct dead_tag {};
	struct lifetime
	{
		lifetime(float value = 10.0f) : value(value) {};

		float value{};
	};

	struct velocity
	{
		velocity() = default;
		velocity(psl::vec3 direction, float force, float inertia) : direction(direction), force(force), inertia(inertia) {};
		psl::vec3 direction;
		float force;
		float inertia;
	};
	struct transform
	{
		transform() = default;
		transform(const psl::vec3& position) : position(position) {};
		transform(const psl::vec3& position, const psl::vec3& scale) : position(position), scale(scale) {};
		psl::quat rotation{1,0,0,0};
		psl::vec3 position{ psl::vec3::zero };
		psl::vec3 scale{ psl::vec3::one };
	};
}