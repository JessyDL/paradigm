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

context::context(const psl::UID& uid, cache& cache, graphics_backend backend, const psl::string8_t& name)
	: m_Backend(backend), m_Handle(cache)
{
	switch(backend)
	{
#ifdef PE_VULKAN
	case graphics_backend::vulkan: m_Handle.load<core::ivk::context>(name); break;
#endif
#ifdef PE_GLES
	case graphics_backend::gles: m_Handle.load<core::igles::context>(name); break;
#endif
	}
}


void context::target_surface(const core::os::surface& surface)
{
#ifdef PE_GLES
	switch(m_Backend)
	{
	case graphics_backend::gles:
		auto handle = m_Handle.get<core::igles::context>();
		handle->enable(surface);
		break;
	}
#endif
}