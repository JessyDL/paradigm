#include "gfx/framebuffer.h"
#include "data/framebuffer.h"
#include "gfx/context.h"
#include "gfx/texture.h"

#ifdef PE_GLES
#include "gles/framebuffer.h"
#endif
#ifdef PE_VULKAN
#include "vk/framebuffer.h"
#endif

using namespace core::resource;
using namespace core::gfx;
using namespace core;


#ifdef PE_VULKAN
framebuffer::framebuffer(core::resource::handle<core::ivk::framebuffer>& handle) :
	m_Backend(graphics_backend::vulkan), m_VKHandle(handle)
{}
#endif
#ifdef PE_GLES
framebuffer::framebuffer(core::resource::handle<core::igles::framebuffer>& handle) :
	m_Backend(graphics_backend::gles), m_GLESHandle(handle)
{}
#endif

framebuffer::framebuffer(core::resource::cache& cache,
						 const core::resource::metadata& metaData,
						 psl::meta::file* metaFile,
						 handle<core::gfx::context> context,
						 handle<data::framebuffer> data) :
	m_Backend(context->backend())
{
	switch(context->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = cache.create_using<core::igles::framebuffer>(metaData.uid, data);
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle =
		  cache.create_using<core::ivk::framebuffer>(metaData.uid, context->resource<graphics_backend::vulkan>(), data);
		break;
#endif
	}
}

texture framebuffer::texture(size_t index) const noexcept
{
#ifdef PE_GLES
	if(m_Backend == graphics_backend::gles) return gfx::texture(m_GLESHandle->color_attachments()[index]);
#endif
#ifdef PE_VULKAN
	if(m_Backend == graphics_backend::vulkan) return gfx::texture {m_VKHandle->color_attachments()[index]};
#endif
	throw std::runtime_error("no backend present");
}