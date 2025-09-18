#pragma once
#include "core/gfx/types.hpp"
#include "core/resource/handle.hpp"
#include "psl/math/math.hpp"

#include "core/gfx/bundle.hpp"
#include "core/gfx/geometry.hpp"

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
	core::gfx::instancing_size_type instance_id {std::numeric_limits<core::gfx::instancing_size_type>::max()};
};

/// \brief Tag that indicates we should upload the transform instance data for this renderable.
struct transform_instance_data_tag {};

/// \brief Tag that indicates we should upload the transform instance object model data for this renderable.
struct transform_instance_object_model_tag {};

struct dont_render_tag {};

}	 // namespace core::ecs::components
