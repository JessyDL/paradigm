#pragma once
#include "fwd/resource/resource.h"
#include "defines.h"


namespace core::gfx
{
	class compute;
} // namespace core::gfx

namespace core::meta
{
	class shader;
}

#ifdef PE_VULKAN
#include "fwd/vk/compute.h"
#endif

#ifdef PE_GLES
#include "fwd/gles/compute.h"
#endif

namespace core::resource
{
	template <>
	struct resource_traits<core::gfx::compute>
	{
		using meta_type = core::meta::shader;
	};
} // namespace core::resource