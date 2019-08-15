#include "gfx/context.h"

#ifdef PE_VULKAN
#include "vk/context.h"
#endif
#ifdef PE_GLES
#include "gles/context.h"
#endif

using namespace core;
using namespace core::gfx;
using namespace core::resource;

context::context(core::resource::handle<value_type>& handle) : m_Handle(handle){};
context::context(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				 graphics_backend backend, const psl::string8_t& name)
	: m_Backend(backend)
{
	switch(backend)
	{
#ifdef PE_VULKAN
	case graphics_backend::vulkan: m_Handle << cache.create_using<core::ivk::context>(metaData.uid, name); break;
#endif
#ifdef PE_GLES
	case graphics_backend::gles: m_Handle << cache.create_using<core::igles::context>(metaData.uid, name); break;
#endif
	}
}