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
sampler_t::sampler_t(core::resource::handle<core::ivk::sampler_t>& handle) :
	m_Backend(graphics_backend::vulkan), m_VKHandle(handle)
{}
#endif
#ifdef PE_GLES
sampler_t::sampler_t(core::resource::handle<core::igles::sampler_t>& handle) :
	m_Backend(graphics_backend::gles), m_GLESHandle(handle)
{}
#endif

sampler_t::sampler_t(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 core::resource::handle<core::gfx::context> context,
				 core::resource::handle<core::data::sampler_t> sampler_data) :
	m_Backend(context->backend())
{
	switch(m_Backend)
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = cache.create_using<core::igles::sampler_t>(metaData.uid, sampler_data);
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle = cache.create_using<core::ivk::sampler_t>(
		  metaData.uid, context->resource<graphics_backend::vulkan>(), sampler_data);
		break;
#endif
	}
}