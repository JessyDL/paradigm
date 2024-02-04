#pragma once
#include "core/fwd/resource/resource.hpp"
#include "core/gfx/types.hpp"

#ifdef PE_VULKAN
namespace core::ivk {
class swapchain;
}
#endif

#ifdef PE_GLES
namespace core::igles {
class swapchain;
}
#endif

#if defined(PE_WEBGPU)
namespace core::iwgpu {
class swapchain;
}
#endif

namespace core::gfx {
class swapchain;


#ifdef PE_VULKAN
template <>
struct backend_type<swapchain, graphics_backend::vulkan> {
	using type = core::ivk::swapchain;
};
#endif
#ifdef PE_GLES
template <>
struct backend_type<swapchain, graphics_backend::gles> {
	using type = core::igles::swapchain;
};
#endif
#if defined(PE_WEBGPU)
template <>
struct backend_type<swapchain, graphics_backend::webgpu> {
	using type = core::iwgpu::swapchain;
};
#endif
}	 // namespace core::gfx
