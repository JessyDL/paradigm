#pragma once
#include "core/fwd/resource/resource.hpp"
#include "core/gfx/types.hpp"

#ifdef PE_VULKAN
namespace core::ivk {
class framebuffer_t;
}
#endif

#ifdef PE_GLES
namespace core::igles {
class framebuffer_t;
}
#endif

namespace core::gfx {
class framebuffer_t;

#ifdef PE_VULKAN

template <>
struct backend_type<framebuffer_t, graphics_backend::vulkan> {
	using type = core::ivk::framebuffer_t;
};
#endif
#ifdef PE_GLES
template <>
struct backend_type<framebuffer_t, graphics_backend::gles> {
	using type = core::igles::framebuffer_t;
};
#endif

}	 // namespace core::gfx
