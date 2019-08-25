#include "gfx/pass.h"
#include "gfx/context.h"
#include "gfx/framebuffer.h"
#include "gfx/swapchain.h"
#include "gfx/drawgroup.h"

#ifdef PE_GLES
#include "gles/pass.h"
#endif
#ifdef PE_VULKAN
#include "vk/pass.h"
#endif

using namespace core::resource;
using namespace core::gfx;
using namespace core;

pass::pass(handle<core::gfx::context> context, handle<core::gfx::framebuffer> framebuffer)
{
	switch(context->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_Handle = new core::igles::pass(framebuffer->resource().get<core::igles::framebuffer>());
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_Handle = new core::ivk::pass(context->resource().get<core::ivk::context>(),
									   framebuffer->resource().get<core::ivk::framebuffer>());
		break;
#endif
	}
}

pass::pass(handle<core::gfx::context> context, handle<core::gfx::swapchain> swapchain)
{
	switch(context->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_Handle = new core::igles::pass(swapchain->resource().get<core::igles::swapchain>());
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_Handle = new core::ivk::pass(context->resource().get<core::ivk::context>(),
									   swapchain->resource().get<core::ivk::swapchain>());
		break;
#endif
	}
}

pass::~pass()
{
	if(m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::pass*>(m_Handle);
		delete(ptr);
#else
		assert(false);
#endif
	}
	else
	{
#ifdef PE_GLES
		auto ptr = std::get<core::igles::pass*>(m_Handle);
		delete(ptr);
#else
		assert(false);
#endif
	}
}


bool pass::is_swapchain() const noexcept
{
	if(m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::pass*>(m_Handle);
		return ptr->is_swapchain();
#else
		assert(false);
#endif
	}
	else
	{
#ifdef PE_GLES
		auto ptr = std::get<core::igles::pass*>(m_Handle);
		return ptr->is_swapchain();
#else
		assert(false);
#endif
	}
}


void pass::prepare()
{
	if(m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::pass*>(m_Handle);
		return ptr->prepare();
#else
		assert(false);
#endif
	}
	else
	{
#ifdef PE_GLES
		auto ptr = std::get<core::igles::pass*>(m_Handle);
		return ptr->prepare();
#else
		assert(false);
#endif
	}
}
bool pass::build()
{
	if(m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::pass*>(m_Handle);
		return ptr->build();
#else
		assert(false);
#endif
	}
	else
	{
#ifdef PE_GLES
		auto ptr = std::get<core::igles::pass*>(m_Handle);
		return ptr->build();
#else
		assert(false);
#endif
	}
}


void pass::clear()
{
	if(m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::pass*>(m_Handle);
		ptr->clear();
#else
		assert(false);
#endif
	}
	else
	{
#ifdef PE_GLES
		auto ptr = std::get<core::igles::pass*>(m_Handle);
		ptr->clear();
#else
		assert(false);
#endif
	}
}
void pass::present()
{
	if(m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::pass*>(m_Handle);
		ptr->present();
#else
		assert(false);
#endif
	}
	else
	{
#ifdef PE_GLES
		auto ptr = std::get<core::igles::pass*>(m_Handle);
		ptr->present();
#else
		assert(false);
#endif
	}
}

bool pass::connect(psl::view_ptr<pass> child) noexcept
{
	if(child->m_Handle.index() != m_Handle.index()) return false;

	if(m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::pass*>(m_Handle);
		ptr->connect(psl::view_ptr<core::ivk::pass>(std::get<core::ivk::pass*>(child->m_Handle)));
		return true;
#else
		assert(false);
#endif
	}
	return false;
}
bool pass::disconnect(psl::view_ptr<pass> child) noexcept
{
	if(child->m_Handle.index() != m_Handle.index()) return false;

	if(m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::pass*>(m_Handle);
		ptr->disconnect(psl::view_ptr<core::ivk::pass>(std::get<core::ivk::pass*>(child->m_Handle)));
		return true;
#else
		assert(false);
#endif
	}
	return false;
}


void pass::add(core::gfx::drawgroup& group) noexcept
{
	if(m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::pass*>(m_Handle);
		ptr->add(group);
#else
		assert(false);
#endif
	}
	else
	{
#ifdef PE_GLES
		auto ptr = std::get<core::igles::pass*>(m_Handle);
		ptr->add(group);
#else
		assert(false);
#endif
	}
}