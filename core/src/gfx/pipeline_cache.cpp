#include "gfx/pipeline_cache.h"
#include "gfx/context.h"

#ifdef PE_GLES
#include "gles/program_cache.h"
#endif
#ifdef PE_VULKAN
#include "vk/pipeline_cache.h"
#endif

using namespace core::resource;
using namespace core::gfx;
using namespace core;

pipeline_cache::pipeline_cache(core::resource::handle<value_type>& handle) : m_Handle(handle){};
pipeline_cache::pipeline_cache(core::resource::cache& cache, const core::resource::metadata& metaData,
							   psl::meta::file* metaFile,
							   core::resource::handle<core::gfx::context> context)
{
	switch(context->backend())
	{
	case graphics_backend::gles: m_Handle << cache.create_using<core::igles::program_cache>(metaData.uid); break;
	case graphics_backend::vulkan:
		m_Handle << cache.create_using<core::ivk::pipeline_cache>(metaData.uid, context->resource().get<core::ivk::context>());
		break;
	}
}