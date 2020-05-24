#pragma once
#include "fwd/resource/resource.h"
#include "gfx/types.h"

#ifdef PE_VULKAN
namespace core::ivk
{
	class material;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class material;
}
#endif

namespace core::gfx
{
	class material;

#ifdef PE_VULKAN
	template<>
	struct backend_type<material, graphics_backend::vulkan>
	{
		using type = core::ivk::material;
	};
#endif
#ifdef PE_GLES
	template<>
	struct backend_type<material, graphics_backend::gles>
	{
		using type = core::igles::material;
	};
#endif
} // namespace core::gfx