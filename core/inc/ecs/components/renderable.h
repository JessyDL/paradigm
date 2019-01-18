#pragma once
#include "math/math.hpp"

namespace core::gfx
{
	class material;
	class geometry;
}
namespace core::ecs::components
{
	struct renderable
	{
		renderable() = default;
		renderable(const core::resource::indirect_handle<core::gfx::material>& material, 
				   const core::resource::indirect_handle<core::gfx::geometry>& geometry, 
				   uint32_t layer) noexcept 
			: material(material), geometry(geometry), layer(layer) {};

		core::resource::indirect_handle<core::gfx::material> material;
		core::resource::indirect_handle<core::gfx::geometry> geometry;
		uint32_t layer;
	};
}