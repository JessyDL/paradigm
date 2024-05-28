#include "core/gfx/context.hpp"

#ifdef PE_VULKAN
	#include "core/vk/context.hpp"
	#include "core/vk/conversion.hpp"
#endif
#ifdef PE_GLES
	#include "core/gles/context.hpp"
	#include "core/gles/igles.hpp"
#endif
#ifdef PE_WEBGPU
	#include "core/wgpu/context.hpp"
	#include "core/wgpu/iwgpu.hpp"
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
#ifdef PE_WEBGPU
context::context(core::resource::handle<core::iwgpu::context>& handle)
	: m_Backend(graphics_backend::webgpu), m_WebGPUHandle(handle) {}
#endif
context::context(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 graphics_backend backend,
				 const psl::string8_t& name,
				 core::resource::handle<core::os::surface> surface)
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
#ifdef PE_WEBGPU
	case graphics_backend::webgpu: {
		m_WebGPUHandle = cache.create_using<core::iwgpu::context>(metaData.uid, name, surface);
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
#ifdef PE_WEBGPU
	if(m_WebGPUHandle)
		return m_WebGPUHandle->limits();
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
#ifdef PE_WEBGPU

#endif
}
