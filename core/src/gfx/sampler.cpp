#include "gfx/sampler.h"
#include "data/sampler.h"
#include "gfx/context.h"

#ifdef PE_VULKAN
#include "vk/sampler.h"
#endif
#ifdef PE_GLES
#include "gles/sampler.h"
#endif

using namespace core::gfx;
using namespace core::resource;
using namespace core;

#ifdef PE_VULKAN
sampler::sampler(core::resource::handle<core::ivk::sampler>& handle) :
	m_Backend(graphics_backend::vulkan), m_VKHandle(handle)
{}
#endif
#ifdef PE_GLES
sampler::sampler(core::resource::handle<core::igles::sampler>& handle) :
	m_Backend(graphics_backend::gles), m_GLESHandle(handle)
{}
#endif

sampler::sampler(core::resource::cache& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 core::resource::handle<core::gfx::context> context,
				 core::resource::handle<core::data::sampler> sampler_data) :
	m_Backend(context->backend())
{
	switch(m_Backend)
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = cache.create_using<core::igles::sampler>(metaData.uid, sampler_data);
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle = cache.create_using<core::ivk::sampler>(
		  metaData.uid, context->resource<graphics_backend::vulkan>(), sampler_data);
		break;
#endif
	}
}