#pragma once
#include "fwd/resource/resource.h"
#include "gfx/types.h"


namespace core::meta
{
	class texture;
}

#ifdef PE_VULKAN
#include "fwd/vk/texture.h"
#endif

#ifdef PE_GLES
#include "fwd/gles/texture.h"
#endif

namespace core::gfx
{
	class texture;

#ifdef PE_VULKAN
	template <>
	struct backend_type<texture, graphics_backend::vulkan>
	{
		using type = core::ivk::texture;
	};
#endif
#ifdef PE_GLES
	template <>
	struct backend_type<texture, graphics_backend::gles>
	{
		using type = core::igles::texture;
	};
#endif
}	 // namespace core::gfx

namespace core::resource
{
	template <>
	struct resource_traits<core::gfx::texture>
	{
		using meta_type = core::meta::texture;
	};
}	 // namespace core::resource