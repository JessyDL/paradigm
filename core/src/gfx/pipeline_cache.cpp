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

pipeline_cache::pipeline_cache(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile,
							   core::resource::handle<core::gfx::context> context)
	: m_Handle(cache, uid, (metaFile) ? metaFile->ID() : uid)
{
	switch(context->backend())
	{
	case graphics_backend::gles: m_Handle.load<core::igles::program_cache>(); break;
	case graphics_backend::vulkan:
		m_Handle.load<core::ivk::pipeline_cache>(context->resource().get<core::ivk::context>());
		break;
	}
}