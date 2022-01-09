#pragma once
#include "defines.h"
#include "fwd/resource/resource.h"
#include "gfx/types.h"

#ifdef PE_VULKAN
namespace core::ivk
{
	class sampler;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class sampler;
}
#endif

namespace core::gfx
{
	class sampler;

#ifdef PE_VULKAN
	template <>
	struct backend_type<sampler, graphics_backend::vulkan>
	{
		using type = core::ivk::sampler;
	};
#endif
#ifdef PE_GLES
	template <>
	struct backend_type<sampler, graphics_backend::gles>
	{
		using type = core::igles::sampler;
	};
#endif
}	 // namespace core::gfx
