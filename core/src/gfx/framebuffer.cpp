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

framebuffer::framebuffer(const psl::UID& uid, cache& cache, psl::meta::file* metaFile,
						 handle<core::gfx::context> context,
						 handle<data::framebuffer> data)
	: m_Handle(cache, uid, (metaFile) ? metaFile->ID() : uid)
{
	switch(context->backend())
	{
	case graphics_backend::gles: break;
	case graphics_backend::vulkan:
		m_Handle.load<core::ivk::framebuffer>(context->resource().get<core::ivk::context>(), data);
		break;
	}
}