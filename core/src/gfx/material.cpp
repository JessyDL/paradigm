#include "gfx/material.h"
#include "data/material.h"
#include "gfx/pipeline_cache.h"
#include "gfx/buffer.h"
#include "gfx/context.h"
#include "psl/memory/segment.h"

#ifdef PE_GLES
#include "gles/material.h"
#endif
#ifdef PE_VULKAN
#include "vk/material.h"
#endif

using namespace core::resource;
using namespace core::gfx;

#ifdef PE_VULKAN
material::material(core::resource::handle<core::ivk::material>& handle)
	: m_Backend(graphics_backend::vulkan), m_VKHandle(handle)
{}
#endif
#ifdef PE_GLES
material::material(core::resource::handle<core::igles::material>& handle)
	: m_Backend(graphics_backend::gles), m_GLESHandle(handle)
{}
#endif

material::material(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				   core::resource::handle<context> context_handle, core::resource::handle<core::data::material> data,
				   core::resource::handle<pipeline_cache> pipeline_cache, core::resource::handle<buffer> materialBuffer)	:m_Backend(context_handle->backend())
{
	switch(m_Backend)
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = cache.create_using<core::igles::material>(metaData.uid, data,
																 pipeline_cache->resource<graphics_backend::gles>());
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle = cache.create_using<core::ivk::material>(
			metaData.uid, context_handle->resource<graphics_backend::vulkan>(), data,
			pipeline_cache->resource<graphics_backend::vulkan>(), materialBuffer->resource<graphics_backend::vulkan>());
		break;
#endif
	}
}

const core::data::material& material::data() const
{
#ifdef PE_GLES
	if(m_GLESHandle)
	{
		return m_GLESHandle->data();
	}
#endif
#ifdef PE_VULKAN
	if(m_VKHandle)
	{
		return m_VKHandle->data().value();
	}
#endif
	throw std::logic_error("core::gfx::material has no API specific material associated with it");
}

bool material::bind_instance_data(uint32_t slot, uint32_t offset)
{
#ifdef PE_GLES
	if(m_GLESHandle)
	{
		return m_GLESHandle->bind_instance_data(slot, offset);
	}
#endif
#ifdef PE_VULKAN
	if(m_VKHandle)
	{
		return m_VKHandle->bind_instance_data(slot, offset);
	}
#endif
	throw std::logic_error("core::gfx::material has no API specific material associated with it");
}