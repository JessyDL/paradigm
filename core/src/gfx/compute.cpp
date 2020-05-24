#include "gfx/compute.h"
#include "gfx/context.h"
#include "gfx/pipeline_cache.h"
#include "data/material.h"

#ifdef PE_GLES
#include "gles/compute.h"
#endif
#ifdef PE_VULKAN
#include "vk/compute.h"
#endif
using namespace core::gfx;
using namespace core::resource;

#ifdef PE_VULKAN
compute::compute(core::resource::handle<core::ivk::compute>& handle)
	: m_Backend(graphics_backend::vulkan), m_VKHandle(handle)
{}
#endif
#ifdef PE_GLES
compute::compute(core::resource::handle<core::igles::compute>& handle)
	: m_Backend(graphics_backend::gles), m_GLESHandle(handle)
{}
#endif

compute::compute(core::resource::cache& cache, const core::resource::metadata& metaData, core::meta::shader* metaFile,
				 core::resource::handle<context> context_handle, core::resource::handle<core::data::material> data,
				 core::resource::handle<pipeline_cache> pipeline_cache)
{
	switch(context_handle->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = cache.create_using<core::igles::compute>(
			metaData.uid, data, pipeline_cache->resource<graphics_backend::gles>());
		break;
#endif
#ifdef PE_VULKAN
		assert(true && "todo implement vulkan backend");
		/*case graphics_backend::vulkan:
			m_Handle << cache.create_using<core::ivk::compute>(metaData.uid,
																context_handle->resource().get<core::ivk::context>(),
		   data, pipeline_cache->resource().get<core::ivk::pipeline_cache>());*/
		break;
#endif
	}
}


void compute::dispatch(const psl::static_array<uint32_t, 3>& size)
{
#ifdef PE_GLES
	if(m_GLESHandle)
	{
		m_GLESHandle->dispatch(size[0], size[1], size[2]);
	}
#endif
#ifdef PE_VULKAN
	if (m_VKHandle)
	{
		m_VKHandle->dispatch(size[0], size[1], size[2]);
	}
#endif
}