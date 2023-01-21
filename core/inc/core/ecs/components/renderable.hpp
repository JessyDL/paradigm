#pragma once
#include "psl/math/math.hpp"
#include "core/resource/handle.hpp"

namespace core::gfx {
class bundle;
class geometry_t;
}	 // namespace core::gfx
namespace core::ecs::components {
struct renderable {
	renderable() = default;
	renderable(const core::resource::handle<core::gfx::bundle>& bundle,
			   const core::resource::handle<core::gfx::geometry_t>& geometry) noexcept
		: bundle(bundle), geometry(geometry) {};

	core::resource::handle<core::gfx::bundle> bundle {};
	core::resource::handle<core::gfx::geometry_t> geometry {};
};

struct dont_render_tag {};
}	 // namespace core::ecs::components
