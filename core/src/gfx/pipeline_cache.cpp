#include "core/gfx/pipeline_cache.hpp"
#include "core/gfx/context.hpp"
#include "core/gfx/types.hpp"

#ifdef PE_GLES
	#include "core/gles/program_cache.hpp"
#endif
#ifdef PE_VULKAN
	#include "core/vk/pipeline_cache.hpp"
#endif

using namespace core::resource;
using namespace core::gfx;
using namespace core;

#ifdef PE_VULKAN
pipeline_cache::pipeline_cache(core::resource::handle<core::ivk::pipeline_cache>& handle)
	: m_Backend(graphics_backend::vulkan), m_VKHandle(handle) {}
#endif
#ifdef PE_GLES
pipeline_cache::pipeline_cache(core::resource::handle<core::igles::program_cache>& handle)
	: m_Backend(graphics_backend::gles), m_GLESHandle(handle) {}
#endif

pipeline_cache::pipeline_cache(core::resource::cache_t& cache,
							   const core::resource::metadata& metaData,
							   psl::meta::file* metaFile,
							   core::resource::handle<core::gfx::context> context)
	: m_Backend(context->backend()) {
	switch(context->backend()) {
	case graphics_backend::gles:
#ifdef PE_GLES
		m_GLESHandle = cache.create_using<core::igles::program_cache>(metaData.uid);
		break;
#endif
		psl::fatal("gles not supported");
	case graphics_backend::vulkan:
#ifdef PE_VULKAN
		m_VKHandle =
		  cache.create_using<core::ivk::pipeline_cache>(metaData.uid, context->resource<graphics_backend::vulkan>());
		break;
#endif
		psl::fatal("vulkan not supported");
	default:
		psl::fatal("Unknown backend not supported");
	}
}
