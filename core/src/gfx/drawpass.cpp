#include "gfx/drawpass.h"
#include "gfx/context.h"
#include "gfx/framebuffer.h"
#include "gfx/swapchain.h"
#include "gfx/drawgroup.h"
#include "gfx/computecall.h"

#ifdef PE_GLES
#include "gles/drawpass.h"
#endif
#ifdef PE_VULKAN
#include "vk/drawpass.h"
#endif

using namespace core::resource;
using namespace core::gfx;
using namespace core;

/// todo: passes cannot describe resources in both API's at the same time, we need to decide if we
///	      want  a feature like it, or if we would prefer to keep only one API going
drawpass::drawpass(handle<core::gfx::context> context, handle<core::gfx::framebuffer> framebuffer)
{
	switch(context->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_Handle = new core::igles::drawpass(framebuffer->resource().get<core::igles::framebuffer>());
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_Handle = new core::ivk::drawpass(context->resource().get<core::ivk::context>(),
									   framebuffer->resource().get<core::ivk::framebuffer>());
		break;
#endif
	}
}

drawpass::drawpass(handle<core::gfx::context> context, handle<core::gfx::swapchain> swapchain)
{
	switch(context->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_Handle = new core::igles::drawpass(swapchain->resource().get<core::igles::swapchain>());
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_Handle = new core::ivk::drawpass(context->resource().get<core::ivk::context>(),
									   swapchain->resource().get<core::ivk::swapchain>());
		break;
#endif
	}
}

drawpass::~drawpass()
{
	std::visit(utility::templates::overloaded{ [](auto&& pass) {return delete(pass); } }, m_Handle);
}


bool drawpass::is_swapchain() const noexcept
{
	return std::visit(utility::templates::overloaded{ [](auto&& pass) {return pass->is_swapchain(); } }, m_Handle);
}


void drawpass::prepare()
{
	return std::visit(utility::templates::overloaded{ [](auto&& pass) {return pass->prepare(); } }, m_Handle);
}
bool drawpass::build(bool force)
{
	if(!m_Dirty && !force) return true;

	m_Dirty = false;
	return std::visit(utility::templates::overloaded{ [](auto&& pass) {return pass->build(); } }, m_Handle);
}


void drawpass::clear()
{
	return std::visit(utility::templates::overloaded{ [](auto&& pass) {return pass->clear(); } }, m_Handle);
}
void drawpass::present()
{
	return std::visit(utility::templates::overloaded{ [](auto&& pass) {return pass->present(); } }, m_Handle);
}

bool drawpass::connect(psl::view_ptr<drawpass> child) noexcept
{
	if(child->m_Handle.index() != m_Handle.index()) return false;

	if(m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::drawpass*>(m_Handle);
		ptr->connect(psl::view_ptr<core::ivk::drawpass>(std::get<core::ivk::drawpass*>(child->m_Handle)));
		return true;
#else
		assert(false);
#endif
	}
	else
	{
#ifdef PE_GLES
		auto ptr = std::get<core::igles::drawpass*>(m_Handle);
		ptr->connect(psl::view_ptr<core::igles::drawpass>(std::get<core::igles::drawpass*>(child->m_Handle)));
		return true;
#else
		assert(false);
#endif

	}
	return false;
}
bool drawpass::disconnect(psl::view_ptr<drawpass> child) noexcept
{
	if(child->m_Handle.index() != m_Handle.index()) return false;

	if(m_Handle.index() == 0)
	{
#ifdef PE_VULKAN
		auto ptr = std::get<core::ivk::drawpass*>(m_Handle);
		ptr->disconnect(psl::view_ptr<core::ivk::drawpass>(std::get<core::ivk::drawpass*>(child->m_Handle)));
		return true;
#else
		assert(false);
#endif
	}
	else
	{
#ifdef PE_GLES
		auto ptr = std::get<core::igles::drawpass*>(m_Handle);
		ptr->disconnect(psl::view_ptr<core::igles::drawpass>(std::get<core::igles::drawpass*>(child->m_Handle)));
		return true;
#else
		assert(false);
#endif

	}
	return false;
}


void drawpass::add(core::gfx::drawgroup& group) noexcept
{
	return std::visit(utility::templates::overloaded{ [&group](auto&& pass) {return pass->add(group); } }, m_Handle);
}