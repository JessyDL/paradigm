#pragma once
#include "defines.hpp"
#include "fwd/resource/resource.hpp"
#include "gfx/types.hpp"

#ifdef PE_VULKAN
namespace core::ivk
{
class sampler_t;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
class sampler_t;
}
#endif

namespace core::gfx
{
class sampler_t;

#ifdef PE_VULKAN
template <>
struct backend_type<sampler_t, graphics_backend::vulkan>
{
	using type = core::ivk::sampler_t;
};
#endif
#ifdef PE_GLES
template <>
struct backend_type<sampler_t, graphics_backend::gles>
{
	using type = core::igles::sampler_t;
};
#endif
}	 // namespace core::gfx
