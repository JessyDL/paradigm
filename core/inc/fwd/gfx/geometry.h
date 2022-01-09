#pragma once
#include "defines.h"
#include "fwd/resource/resource.h"
#include "gfx/types.h"

#ifdef PE_VULKAN
namespace core::ivk
{
	class geometry;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class geometry;
}
#endif

namespace core::gfx
{
	class geometry;

#ifdef PE_VULKAN
	template <>
	struct backend_type<geometry, graphics_backend::vulkan>
	{
		using type = core::ivk::geometry;
	};
#endif
#ifdef PE_GLES
	template <>
	struct backend_type<geometry, graphics_backend::gles>
	{
		using type = core::igles::geometry;
	};
#endif
}	 // namespace core::gfx