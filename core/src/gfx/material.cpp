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

material::material(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile,
				   core::resource::handle<context> context_handle, core::resource::handle<core::data::material> data,
				   core::resource::handle<pipeline_cache> pipeline_cache, core::resource::handle<buffer> materialBuffer)
	: m_Handle(cache, uid, (metaFile) ? metaFile->ID() : uid)
{
	switch(context_handle->backend())
	{
	case graphics_backend::gles:
		m_Handle.load<core::igles::material>(data, pipeline_cache->resource().get<core::igles::program_cache>(),
											 materialBuffer->resource().get<core::igles::buffer>());
		break;
	case graphics_backend::vulkan:
		m_Handle.load<core::ivk::material>(context_handle->resource().get<core::ivk::context>(), data,
											 pipeline_cache->resource().get<core::ivk::pipeline_cache>(),
											 materialBuffer->resource().get<core::ivk::buffer>());
		break;
	}
}