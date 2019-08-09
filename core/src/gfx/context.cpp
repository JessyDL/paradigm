#include "gfx/context.h"

using namespace core;
using namespace core::gfx;
using namespace core::resource;

context::context(const psl::UID& uid, cache& cache, graphics_backend backend, const psl::string8_t& name)
	: m_Backend(backend), m_Handle(cache)
{
	switch(backend)
	{
	case graphics_backend::vulkan: m_Handle.load<core::ivk::context>(name); break;
	case graphics_backend::gles: m_Handle.load<core::igles::context>(name); break;
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