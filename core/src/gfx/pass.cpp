#include "gfx/pass.h"
#include "gfx/context.h"
#include "gfx/framebuffer.h"
#include "gfx/swapchain.h"

#ifdef PE_GLES
#include "gles/pass.h"
#endif
#ifdef PE_VULKAN
#include "vk/pass.h"
#endif

using namespace core::resource;
using namespace core::gfx;
using namespace core;

pass::pass(handle<core::gfx::context> context, handle<core::gfx::framebuffer> framebuffer)
{
	switch(context->backend())
	{
	case graphics_backend::gles: 

		break;
	case graphics_backend::vulkan:
		m_Handle = new core::ivk::pass(context->resource().get<core::ivk::context>(), framebuffer->resource().get<core::ivk::framebuffer>());
		break;
	}
}

pass::pass(handle<core::gfx::context> context, handle<core::gfx::swapchain> swapchain)
{
	switch(context->backend())
	{
	case graphics_backend::gles:
		m_Handle = new core::igles::pass(swapchain->resource().get<core::igles::swapchain>());
		break;
	case graphics_backend::vulkan:
		m_Handle = new core::ivk::pass(context->resource().get<core::ivk::context>(),
									   swapchain->resource().get<core::ivk::swapchain>());
		break;
	}
}