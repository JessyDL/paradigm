#pragma once
#include "defines.hpp"
#include "core/fwd/resource/resource.hpp"
#include "gfx/types.hpp"

#ifdef PE_VULKAN
namespace core::ivk {
class geometry_t;
}
#endif

#ifdef PE_GLES
namespace core::igles {
class geometry_t;
}
#endif

namespace core::gfx {
class geometry_t;

#ifdef PE_VULKAN
template <>
struct backend_type<geometry_t, graphics_backend::vulkan> {
	using type = core::ivk::geometry_t;
};
#endif
#ifdef PE_GLES
template <>
struct backend_type<geometry_t, graphics_backend::gles> {
	using type = core::igles::geometry_t;
};
#endif
}	 // namespace core::gfx
