#pragma once
#include "math/math.hpp"

namespace core::ivk
{
	class geometry;
}

namespace core::gfx
{
	class bundle;
}
namespace core::ecs::components
{
	struct renderable
	{
		renderable() = default;
		renderable(const core::resource::indirect_handle<core::gfx::bundle>& bundle,
				   const core::resource::indirect_handle<core::ivk::geometry>& geometry) noexcept
			: bundle(bundle), geometry(geometry){};

		core::resource::indirect_handle<core::gfx::bundle> bundle{};
		core::resource::indirect_handle<core::ivk::geometry> geometry{};
	};
} // namespace core::ecs::components