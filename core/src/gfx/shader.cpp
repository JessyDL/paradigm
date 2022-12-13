#include "gfx/shader.hpp"
#include "gfx/context.hpp"
#include "meta/shader.hpp"

#ifdef PE_VULKAN
	#include "vk/shader.hpp"
#endif
#ifdef PE_GLES
	#include "gles/shader.hpp"
#endif

using namespace core;
using namespace core::gfx;
using namespace core::resource;


#ifdef PE_VULKAN
shader::shader(core::resource::handle<core::ivk::shader>& handle) :
	m_Backend(graphics_backend::vulkan), m_VKHandle(handle)
{}
#endif
#ifdef PE_GLES
shader::shader(core::resource::handle<core::igles::shader>& handle) :
	m_Backend(graphics_backend::gles), m_GLESHandle(handle)
{}
#endif

shader::shader(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   core::meta::shader* metaFile,
			   core::resource::handle<core::gfx::context> context) :
	m_Backend(context->backend())
{
	switch(context->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = cache.create_using<core::igles::shader>(metaData.uid);
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle = cache.create_using<core::ivk::shader>(metaData.uid, context->resource<graphics_backend::vulkan>());
		break;
#endif
	}
}


core::meta::shader* shader::meta() const noexcept
{
	switch(m_Backend)
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		return m_GLESHandle.meta();
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		return m_VKHandle.meta();
		break;
#endif
	}
	return nullptr;
}
