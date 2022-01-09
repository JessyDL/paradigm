#include "gfx/texture.h"
#include "gfx/context.h"
#include "meta/texture.h"

#ifdef PE_VULKAN
#include "vk/texture.h"
#endif
#ifdef PE_GLES
#include "gles/texture.h"
#endif

using namespace core;
using namespace core::gfx;
using namespace core::resource;


#ifdef PE_VULKAN
texture::texture(core::resource::handle<core::ivk::texture>& handle) :
	m_Backend(graphics_backend::vulkan), m_VKHandle(handle)
{}
#endif
#ifdef PE_GLES
texture::texture(core::resource::handle<core::igles::texture>& handle) :
	m_Backend(graphics_backend::gles), m_GLESHandle(handle)
{}
#endif

texture::texture(core::resource::cache& cache,
				 const core::resource::metadata& metaData,
				 core::meta::texture* metaFile,
				 core::resource::handle<core::gfx::context> context) :
	m_Backend(context->backend())
{
	switch(m_Backend)
	{
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle =
		  cache.create_using<core::ivk::texture>(metaData.uid, context->resource<graphics_backend::vulkan>());
		break;
#endif
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = cache.create_using<core::igles::texture>(metaData.uid);
		break;
#endif
	}
}

texture::~texture() {}

const core::meta::texture& texture::meta() const noexcept
{
#ifdef PE_VULKAN
	if(m_Backend == graphics_backend::vulkan) return m_VKHandle->meta();
#endif
#ifdef PE_GLES
	if(m_Backend == graphics_backend::gles) return m_GLESHandle->meta();
#endif
}