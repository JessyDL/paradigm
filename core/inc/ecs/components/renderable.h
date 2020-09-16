#pragma once
#include "psl/math/math.hpp"

namespace core::gfx
{
	class bundle;
	class geometry;
}
namespace core::ecs::components
{
	struct renderable
	{
		renderable() = default;
		renderable(const core::resource::handle<core::gfx::bundle>& bundle,
				   const core::resource::handle<core::gfx::geometry>& geometry) noexcept
			: bundle(bundle), geometry(geometry) {};

		core::resource::handle<core::gfx::bundle> bundle{};
		core::resource::handle<core::gfx::geometry> geometry{};
	};

	struct dont_render_tag {};
} // namespace core::ecs::components