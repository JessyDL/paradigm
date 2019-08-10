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

swapchain::swapchain(const psl::UID& uid, cache& cache, psl::meta::file* metaFile, handle<os::surface> surface,
					 handle<context> context, bool use_depth)
	: m_Handle(cache, uid, (metaFile) ? metaFile->ID() : uid)
{
	switch(context->backend())
	{
	case graphics_backend::gles:
		m_Handle.load<core::igles::swapchain>(surface, context->resource().get < core::igles::context>(), use_depth);
		break;
	case graphics_backend::vulkan:
		m_Handle.load<core::ivk::swapchain>(surface, context->resource().get<core::ivk::context>(), use_depth);
		surface->register_swapchain(m_Handle.get<core::ivk::swapchain>());
		break;
	}
}