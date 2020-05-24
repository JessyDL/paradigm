#include "gfx/geometry.h"
#include "gfx/context.h"
#include "gfx/buffer.h"

#ifdef PE_GLES
#include "gles/geometry.h"
#endif

#ifdef PE_VULKAN
#include "vk/geometry.h"
#endif
using namespace core;
using namespace core::gfx;
using namespace core::resource;

#ifdef PE_VULKAN
geometry::geometry(core::resource::handle<core::ivk::geometry>& handle)
	: m_Backend(graphics_backend::vulkan), m_VKHandle(handle)
{}
#endif
#ifdef PE_GLES
geometry::geometry(core::resource::handle<core::igles::geometry>& handle)
	: m_Backend(graphics_backend::gles), m_GLESHandle(handle)
{}
#endif

geometry::geometry(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				   core::resource::handle<context> context, core::resource::handle<core::data::geometry> data,
				   core::resource::handle<buffer> geometryBuffer, core::resource::handle<buffer> indicesBuffer)
	: m_Backend(context->backend())
{
	switch(m_Backend)
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = cache.create_using<core::igles::geometry>(metaData.uid, data,
																 geometryBuffer->resource<graphics_backend::gles>(),
																 indicesBuffer->resource<graphics_backend::gles>());
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle = cache.create_using<core::ivk::geometry>(
			metaData.uid, context->resource<graphics_backend::vulkan>(), data,
			geometryBuffer->resource<graphics_backend::vulkan>(), indicesBuffer->resource<graphics_backend::vulkan>());
		break;
#endif
	}
}

geometry::~geometry() {}

void geometry::recreate(core::resource::handle<core::data::geometry> data)
{
#ifdef PE_GLES
	if(m_GLESHandle)
	{
		m_GLESHandle->recreate(data);
	}

#endif
#ifdef PE_VULKAN
	if(m_VKHandle)
	{
		m_VKHandle->recreate(data);
	}
#endif
}

void geometry::recreate(core::resource::handle<core::data::geometry> data,
						core::resource::handle<core::gfx::buffer> geometryBuffer,
						core::resource::handle<core::gfx::buffer> indicesBuffer)
{
#ifdef PE_GLES
	if(m_GLESHandle && geometryBuffer->resource<graphics_backend::gles>() &&
	   indicesBuffer->resource<graphics_backend::gles>())
	{
		m_GLESHandle->recreate(data, geometryBuffer->resource<graphics_backend::gles>(),
							   indicesBuffer->resource<graphics_backend::gles>());
	}

#endif
#ifdef PE_VULKAN
	if(m_VKHandle && geometryBuffer->resource<graphics_backend::vulkan>() &&
	   indicesBuffer->resource<graphics_backend::vulkan>())
	{
		m_VKHandle->recreate(data, geometryBuffer->resource<graphics_backend::vulkan>(),
							 indicesBuffer->resource<graphics_backend::vulkan>());
	}
#endif
}