#pragma once
#include "fwd/resource/resource.h"

namespace core::gfx
{
	class texture;
} // namespace core::gfx

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

namespace core::resource
{
	template <>
	struct resource_traits<core::gfx::texture>
	{
		using meta_type = core::meta::texture;
	};
} // namespace core::resource