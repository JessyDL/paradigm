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


swapchain::swapchain(core::resource::handle<value_type>& handle) : m_Handle(handle){};
swapchain::swapchain(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
					 handle<os::surface> surface, handle<context> context, bool use_depth)
{
	switch(context->backend())
	{
	case graphics_backend::gles:
	{
		//auto igles_context = context->resource().get<core::igles::context>();
		m_Handle << cache.create_using<core::igles::swapchain>(
			metaData.uid, surface, context->resource().get<core::igles::context>(), use_depth);
	}
	break;
	case graphics_backend::vulkan:
		m_Handle << cache.create_using<core::ivk::swapchain>(metaData.uid, surface,
															 context->resource().get<core::ivk::context>(), use_depth);
		surface->register_swapchain(m_Handle.get<core::ivk::swapchain>());
		break;
	}

}