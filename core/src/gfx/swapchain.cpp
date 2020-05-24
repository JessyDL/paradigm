#include "gfx/swapchain.h"
#include "gfx/context.h"
#include "os/surface.h"

#ifdef PE_VULKAN
#include "vk/swapchain.h"
#endif
#ifdef PE_GLES
#include "gles/swapchain.h"
#endif

using namespace core::gfx;
using namespace core::resource;
using namespace core;

#ifdef PE_VULKAN
swapchain::swapchain(core::resource::handle<core::ivk::swapchain>& handle)
	: m_Backend(graphics_backend::vulkan), m_VKHandle(handle)
{}
#endif
#ifdef PE_GLES
swapchain::swapchain(core::resource::handle<core::igles::swapchain>& handle)
	: m_Backend(graphics_backend::gles), m_GLESHandle(handle)
{}
#endif

swapchain::swapchain(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
					 handle<os::surface> surface, handle<context> context, bool use_depth)
	: m_Backend(context->backend())
{
	switch(context->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles:
	{
		// auto igles_context = context->resource().get<core::igles::context>();
		m_GLESHandle = cache.create_using<core::igles::swapchain>(
			metaData.uid, surface, context->resource< graphics_backend::gles>(), use_depth);
	}
	break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle = cache.create_using<core::ivk::swapchain>(metaData.uid, surface,
															  context->resource< graphics_backend::vulkan>(), use_depth);
		surface->register_swapchain(m_VKHandle);
		break;
#endif
	}
}