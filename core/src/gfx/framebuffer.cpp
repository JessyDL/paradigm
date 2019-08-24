#include "gfx/framebuffer.h"
#include "gfx/context.h"
#include "data/framebuffer.h"

#ifdef PE_GLES
#include "gles/framebuffer.h"
#endif
#ifdef PE_VULKAN
#include "vk/framebuffer.h"
#endif

using namespace core::resource;
using namespace core::gfx;
using namespace core;

framebuffer::framebuffer(core::resource::handle<value_type>& handle) : m_Handle(handle){};
framebuffer::framebuffer(core::resource::cache& cache, const core::resource::metadata& metaData,
						 psl::meta::file* metaFile,
						 handle<core::gfx::context> context,
						 handle<data::framebuffer> data)
{
	switch(context->backend())
	{
	case graphics_backend::gles:
		m_Handle << cache.create_using<core::igles::framebuffer>(metaData.uid, data);
		break;
	case graphics_backend::vulkan:
		m_Handle << cache.create_using<core::ivk::framebuffer>(metaData.uid, context->resource().get<core::ivk::context>(), data);
		break;
	}
}