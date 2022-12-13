#pragma once
#include "fwd/resource/resource.hpp"
#include "gfx/types.hpp"

#ifdef PE_VULKAN
namespace core::ivk
{
class material_t;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
class material_t;
}
#endif

namespace core::gfx
{
class material_t;

#ifdef PE_VULKAN
template <>
struct backend_type<material_t, graphics_backend::vulkan>
{
	using type = core::ivk::material_t;
};
#endif
#ifdef PE_GLES
template <>
struct backend_type<material_t, graphics_backend::gles>
{
	using type = core::igles::material_t;
};
#endif
}	 // namespace core::gfx