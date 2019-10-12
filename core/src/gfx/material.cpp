#include "gfx/material.h"
#include "data/material.h"
#include "gfx/pipeline_cache.h"
#include "gfx/buffer.h"
#include "gfx/context.h"

#ifdef PE_GLES
#include "gles/material.h"
#endif
#ifdef PE_VULKAN
#include "vk/material.h"
#endif

using namespace core::resource;
using namespace core::gfx;

material::material(core::resource::handle<value_type>& handle) : m_Handle(handle){};
material::material(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				   core::resource::handle<context> context_handle, core::resource::handle<core::data::material> data,
				   core::resource::handle<pipeline_cache> pipeline_cache, core::resource::handle<buffer> materialBuffer)
{
	switch(context_handle->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_Handle << cache.create_using<core::igles::material>(
			metaData.uid, data, pipeline_cache->resource().get<core::igles::program_cache>(),
			materialBuffer->resource().get<core::igles::buffer>());
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_Handle << cache.create_using<core::ivk::material>(metaData.uid,
															context_handle->resource().get<core::ivk::context>(), data,
															pipeline_cache->resource().get<core::ivk::pipeline_cache>(),
															materialBuffer->resource().get<core::ivk::buffer>());
		break;
#endif
	}
}

const core::data::material& material::data() const
{
#ifdef PE_GLES
	if(m_Handle.contains<igles::material>())
	{
		return m_Handle.value<igles::material>().data();
	}
#endif
#ifdef PE_VULKAN
	if(m_Handle.contains<ivk::material>())
	{
		return m_Handle.value<ivk::material>().data().value();
	}
#endif
	throw std::logic_error("core::gfx::material has no API specific material associated with it");
}