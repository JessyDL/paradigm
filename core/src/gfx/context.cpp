#include "core/gfx/context.hpp"

#ifdef PE_VULKAN
	#include "core/vk/context.hpp"
	#include "core/vk/conversion.hpp"
#endif
#ifdef PE_GLES
	#include "core/gles/context.hpp"
	#include "core/gles/igles.hpp"
#endif

using namespace core;
using namespace core::gfx;
using namespace core::resource;

#ifdef PE_VULKAN
context::context(core::resource::handle<core::ivk::context>& handle)
	: m_Backend(graphics_backend::vulkan), m_VKHandle(handle) {}
#endif
#ifdef PE_GLES
context::context(core::resource::handle<core::igles::context>& handle)
	: m_Backend(graphics_backend::gles), m_GLESHandle(handle) {}
#endif
context::context(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 graphics_backend backend,
				 const psl::string8_t& name)
	: m_Backend(backend) {
	switch(backend) {
#ifdef PE_VULKAN
	case graphics_backend::vulkan: {
		m_VKHandle = cache.create_using<core::ivk::context>(metaData.uid, name);
	} break;
#endif
#ifdef PE_GLES
	case graphics_backend::gles: {
		m_GLESHandle = cache.create_using<core::igles::context>(metaData.uid, name);
	} break;
#endif
	}
}

[[noreturn]] void fail() {
	throw std::runtime_error("no context was loaded");
}

const core::gfx::limits& context::limits() const noexcept {
#ifdef PE_VULKAN
	if(m_VKHandle)
		return m_VKHandle->limits();
#endif
#ifdef PE_GLES
	if(m_GLESHandle)
		return m_GLESHandle->limits();
#endif
	fail();
}

void context::wait_idle() {
#ifdef PE_VULKAN
	if(m_VKHandle) {
		m_VKHandle->device().waitIdle();
		return;
	}
#endif
#ifdef PE_GLES

#endif
}
