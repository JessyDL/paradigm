#pragma once

namespace core::ecs::components
{
	struct lifetime
	{
		lifetime(float value = 10.0f) : value(value) {};

		float value {};
	};
}	 // namespace core::ecs::components