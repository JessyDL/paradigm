#include "gfx/computepass.hpp"
#include "gfx/computecall.hpp"
#include "gfx/context.hpp"

#if PE_GLES
	#include "gles/computepass.hpp"
#endif
#if PE_VULKAN
	#include "vk/computepass.hpp"
#endif

using namespace core::resource;
using namespace core::gfx;
using namespace core;

#ifdef PE_VULKAN
computepass::computepass(core::ivk::computepass* handle) : m_Backend(graphics_backend::vulkan), m_VKHandle(handle) {}
#endif
#ifdef PE_GLES
computepass::computepass(core::igles::computepass* handle) : m_Backend(graphics_backend::gles), m_GLESHandle(handle) {}
#endif


computepass::computepass(handle<core::gfx::context> context) : m_Backend(context->backend()) {
	switch(m_Backend) {
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = new core::igles::computepass();
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle = new core::ivk::computepass();
		break;
#endif
	}
}

computepass::~computepass() {
#ifdef PE_GLES
	delete(m_GLESHandle);
#endif
#ifdef PE_VULKAN
	delete(m_VKHandle);
#endif
}


void computepass::prepare() {
#ifdef PE_GLES
	if(m_GLESHandle)
		m_GLESHandle->prepare();
#endif
#ifdef PE_VULKAN
	if(m_VKHandle)
		m_VKHandle->prepare();
#endif
}
bool computepass::build(bool force) {
	if(!m_Dirty && !force)
		return true;

	m_Dirty = false;
#ifdef PE_GLES
	if(m_GLESHandle)
		m_GLESHandle->build();
#endif
#ifdef PE_VULKAN
	if(m_VKHandle)
		m_VKHandle->build();
#endif
	return false;
}


void computepass::clear() {
#ifdef PE_GLES
	if(m_GLESHandle)
		m_GLESHandle->clear();
#endif
#ifdef PE_VULKAN
	if(m_VKHandle)
		m_VKHandle->clear();
#endif
}
void computepass::present() {
#ifdef PE_GLES
	if(m_GLESHandle)
		m_GLESHandle->present();
#endif
#ifdef PE_VULKAN
	if(m_VKHandle)
		m_VKHandle->present();
#endif
}


bool computepass::connect(psl::view_ptr<computepass> child) noexcept {
#ifdef PE_VULKAN
	if(m_VKHandle) {
		m_VKHandle->connect(child->m_VKHandle);
		return true;
	}
#endif
#ifdef PE_GLES
	if(m_VKHandle) {
		m_GLESHandle->connect(child->m_GLESHandle);
		return true;
	}
#endif
	return false;
}
bool computepass::disconnect(psl::view_ptr<computepass> child) noexcept {
#ifdef PE_VULKAN
	if(m_VKHandle) {
		m_VKHandle->disconnect(child->m_VKHandle);
		return true;
	}
#endif
#ifdef PE_GLES
	if(m_VKHandle) {
		m_GLESHandle->disconnect(child->m_GLESHandle);
		return true;
	}
#endif
	return false;
}


void computepass::add(const core::gfx::computecall& call) noexcept {
#ifdef PE_GLES
	if(m_GLESHandle)
		m_GLESHandle->add(call);
#endif
#ifdef PE_VULKAN
	if(m_VKHandle)
		m_VKHandle->add(call);
#endif
}
