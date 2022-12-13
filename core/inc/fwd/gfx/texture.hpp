#pragma once
#include "fwd/resource/resource.hpp"
#include "gfx/types.hpp"


namespace core::meta
{
class texture_t;
}

#ifdef PE_VULKAN
	#include "fwd/vk/texture.hpp"
#endif

#ifdef PE_GLES
	#include "fwd/gles/texture.hpp"
#endif

namespace core::gfx
{
class texture_t;

#ifdef PE_VULKAN
template <>
struct backend_type<texture_t, graphics_backend::vulkan>
{
	using type = core::ivk::texture_t;
};
#endif
#ifdef PE_GLES
template <>
struct backend_type<texture_t, graphics_backend::gles>
{
	using type = core::igles::texture_t;
};
#endif
}	 // namespace core::gfx

namespace core::resource
{
template <>
struct resource_traits<core::gfx::texture_t>
{
	using meta_type = core::meta::texture_t;
};
}	 // namespace core::resource