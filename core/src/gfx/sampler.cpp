#include "gfx/sampler.h"
#include "gfx/context.h"
#include "data/sampler.h"

#ifdef PE_VULKAN
#include "vk/sampler.h"
#endif
#ifdef PE_GLES
#include "gles/sampler.h"
#endif

using namespace core::gfx;
using namespace core::resource;
using namespace core;

sampler::sampler(core::resource::handle<value_type>& handle) : m_Handle(handle){};
sampler::sampler(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				 core::resource::handle<core::gfx::context> context,
				 core::resource::handle<core::data::sampler> sampler_data)
{
	switch(context->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles: m_Handle << cache.create_using<core::igles::sampler>(metaData.uid, sampler_data); break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_Handle << cache.create_using<core::ivk::sampler>(metaData.uid, context->resource().get<core::ivk::context>(), sampler_data);
		break;
#endif
	}
}