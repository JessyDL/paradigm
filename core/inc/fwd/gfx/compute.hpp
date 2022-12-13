#pragma once
#include "defines.hpp"
#include "fwd/resource/resource.hpp"


namespace core::gfx
{
class compute;
}	 // namespace core::gfx

namespace core::meta
{
class shader;
}

#ifdef PE_VULKAN
	#include "fwd/vk/compute.hpp"
#endif

#ifdef PE_GLES
	#include "fwd/gles/compute.hpp"
#endif

namespace core::resource
{
template <>
struct resource_traits<core::gfx::compute>
{
	using meta_type = core::meta::shader;
};
}	 // namespace core::resource
