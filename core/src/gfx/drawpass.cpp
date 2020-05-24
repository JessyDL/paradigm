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

#ifdef PE_VULKAN
drawpass::drawpass(core::ivk::drawpass* handle) : m_Backend(graphics_backend::vulkan), m_VKHandle(handle) {}
#endif
#ifdef PE_GLES
drawpass::drawpass(core::igles::drawpass* handle) : m_Backend(graphics_backend::gles), m_GLESHandle(handle) {}
#endif

/// todo: passes cannot describe resources in both API's at the same time, we need to decide if we
///	      want  a feature like it, or if we would prefer to keep only one API going
drawpass::drawpass(handle<core::gfx::context> context, handle<core::gfx::framebuffer> framebuffer)
	: m_Backend(context->backend())
{
	switch(m_Backend)
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = new core::igles::drawpass(framebuffer->resource<graphics_backend::gles>());
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle = new core::ivk::drawpass(context->resource<graphics_backend::vulkan>(),
											 framebuffer->resource<graphics_backend::vulkan>());
		break;
#endif
	}
}

drawpass::drawpass(handle<core::gfx::context> context, handle<core::gfx::swapchain> swapchain)
	: m_Backend(context->backend())
{
	switch(m_Backend)
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = new core::igles::drawpass(swapchain->resource<graphics_backend::gles>());
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle = new core::ivk::drawpass(context->resource<graphics_backend::vulkan>(),
											 swapchain->resource<graphics_backend::vulkan>());
		break;
#endif
	}
}

drawpass::~drawpass()
{
#ifdef PE_GLES
	delete(m_GLESHandle);
#endif
#ifdef PE_VULKAN
	delete(m_VKHandle);
#endif
}


bool drawpass::is_swapchain() const noexcept
{
#ifdef PE_GLES
	if(m_GLESHandle) return m_GLESHandle->is_swapchain();
#endif
#ifdef PE_VULKAN
	if(m_VKHandle)return m_VKHandle->is_swapchain();
#endif
	return false;
}


void drawpass::prepare()
{
#ifdef PE_GLES
	if(m_GLESHandle) m_GLESHandle->prepare();
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) m_VKHandle->prepare();
#endif
}
bool drawpass::build(bool force)
{
	if(!m_Dirty && !force) return true;

	m_Dirty = false;
#ifdef PE_GLES
	if(m_GLESHandle) return m_GLESHandle->build();
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) return m_VKHandle->build();
#endif
}


void drawpass::clear()
{
#ifdef PE_GLES
	if(m_GLESHandle) m_GLESHandle->clear();
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) m_VKHandle->clear();
#endif
}
void drawpass::present()
{
#ifdef PE_GLES
	if(m_GLESHandle) m_GLESHandle->present();
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) m_VKHandle->present();
#endif
}

bool drawpass::connect(psl::view_ptr<drawpass> child) noexcept
{
#ifdef PE_VULKAN
	if(m_VKHandle)
	{
		m_VKHandle->connect(psl::view_ptr<core::ivk::drawpass>(child->m_VKHandle));
		return true;
	}
#endif
#ifdef PE_GLES
	if(m_VKHandle)
	{
		m_GLESHandle->connect(psl::view_ptr<core::igles::drawpass>(child->m_GLESHandle));
		return true;
	}
#endif
	return false;
}
bool drawpass::disconnect(psl::view_ptr<drawpass> child) noexcept
{
#ifdef PE_VULKAN
	if (m_VKHandle)
	{
		m_VKHandle->disconnect(psl::view_ptr<core::ivk::drawpass>(child->m_VKHandle));
		return true;
	}
#endif
#ifdef PE_GLES
	if (m_VKHandle)
	{
		m_GLESHandle->disconnect(psl::view_ptr<core::igles::drawpass>(child->m_GLESHandle));
		return true;
	}
#endif
	return false;
}


void drawpass::add(core::gfx::drawgroup& group) noexcept
{
#ifdef PE_GLES
	if(m_GLESHandle) m_GLESHandle->add(group);
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) m_VKHandle->add(group);
#endif
}