#pragma once
#include "psl/math/math.hpp"

namespace core::ecs::components
{
struct transform
{
	transform() = default;
	transform(const psl::vec3& position) : position(position) {};
	transform(const psl::vec3& position, const psl::vec3& scale) : position(position), scale(scale) {};
	transform(psl::vec3 position, psl::vec3 scale, psl::quat rotation) :
		position(std::move(position)), scale(std::move(scale)), rotation(std::move(rotation)) {};
	psl::quat rotation {psl::quat::identity};
	psl::vec3 position {psl::vec3::zero};
	psl::vec3 scale {psl::vec3::one};
};

struct dynamic_tag
{};
}	 // namespace core::ecs::components