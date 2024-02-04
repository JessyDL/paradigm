#include "core/gfx/swapchain.hpp"
#include "core/gfx/context.hpp"
#include "core/os/surface.hpp"

#ifdef PE_VULKAN
	#include "core/vk/swapchain.hpp"
#endif
#ifdef PE_GLES
	#include "core/gles/swapchain.hpp"
#endif
#if defined(PE_WEBGPU)
	#include "core/wgpu/swapchain.hpp"
#endif

using namespace core::gfx;
using namespace core::resource;
using namespace core;

#ifdef PE_VULKAN
swapchain::swapchain(core::resource::handle<core::ivk::swapchain>& handle)
	: m_Backend(graphics_backend::vulkan), m_VKHandle(handle) {}
#endif
#ifdef PE_GLES
swapchain::swapchain(core::resource::handle<core::igles::swapchain>& handle)
	: m_Backend(graphics_backend::gles), m_GLESHandle(handle) {}
#endif
#if defined(PE_WEBGPU)
swapchain::swapchain(core::resource::handle<core::iwgpu::swapchain>& handle)
	: m_Backend(graphics_backend::webgpu), m_WGPUHandle(handle) {}
#endif

swapchain::swapchain(core::resource::cache_t& cache,
					 const core::resource::metadata& metaData,
					 psl::meta::file* metaFile,
					 handle<os::surface> surface,
					 handle<context> context,
					 [[maybe_unused]] core::os::context& os_context,
					 bool use_depth)
	: m_Backend(context->backend()) {
	switch(context->backend()) {
#ifdef PE_GLES
	case graphics_backend::gles: {
		// auto igles_context = context->resource().get<core::igles::context>();
		m_GLESHandle = cache.create_using<core::igles::swapchain>(
		  metaData.uid, surface, context->resource<graphics_backend::gles>(), use_depth);
	} break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle = cache.create_using<core::ivk::swapchain>(
		  metaData.uid, surface, context->resource<graphics_backend::vulkan>(), os_context, use_depth);
		surface->register_swapchain(m_VKHandle);
		break;
#endif
#if defined(PE_WEBGPU)
	case graphics_backend::webgpu:
		m_WGPUHandle = cache.create_using<core::iwgpu::swapchain>(
		  metaData.uid, surface, context->resource<graphics_backend::webgpu>(), use_depth);
		break;
#endif
	default:
		throw std::runtime_error("a backend that was not enabled somehow was used to create a context.");
	}
}
