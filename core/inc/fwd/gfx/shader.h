#pragma once
#include "fwd/resource/resource.h"
#include "defines.h"


namespace core::gfx
{
	class shader;
} // namespace core::gfx

namespace core::meta
{
	class shader;
}

#ifdef PE_VULKAN
#include "fwd/vk/shader.h"
#endif

#ifdef PE_GLES
#include "fwd/gles/shader.h"
#endif

namespace core::resource
{
	template <>
	struct resource_traits<core::gfx::shader>
	{
		using meta_type = core::meta::shader;
	};
} // namespace core::resource