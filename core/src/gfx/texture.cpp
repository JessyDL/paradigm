#include "gfx/texture.hpp"
#include "gfx/context.hpp"
#include "meta/texture.hpp"

#ifdef PE_VULKAN
	#include "vk/texture.hpp"
#endif
#ifdef PE_GLES
	#include "gles/texture.hpp"
#endif

using namespace core;
using namespace core::gfx;
using namespace core::resource;


#ifdef PE_VULKAN
texture_t::texture_t(core::resource::handle<core::ivk::texture_t>& handle)
	: m_Backend(graphics_backend::vulkan), m_VKHandle(handle) {}
#endif
#ifdef PE_GLES
texture_t::texture_t(core::resource::handle<core::igles::texture_t>& handle)
	: m_Backend(graphics_backend::gles), m_GLESHandle(handle) {}
#endif

texture_t::texture_t(core::resource::cache_t& cache,
					 const core::resource::metadata& metaData,
					 core::meta::texture_t* metaFile,
					 core::resource::handle<core::gfx::context> context)
	: m_Backend(context->backend()) {
	switch(m_Backend) {
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle =
		  cache.create_using<core::ivk::texture_t>(metaData.uid, context->resource<graphics_backend::vulkan>());
		break;
#endif
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = cache.create_using<core::igles::texture_t>(metaData.uid);
		break;
#endif
	}
}

texture_t::~texture_t() {}

[[noreturn]] void fail_backend() {
	throw std::runtime_error("no backend present");
}

const core::meta::texture_t& texture_t::meta() const noexcept {
#ifdef PE_VULKAN
	if(m_Backend == graphics_backend::vulkan)
		return m_VKHandle->meta();
#endif
#ifdef PE_GLES
	if(m_Backend == graphics_backend::gles)
		return m_GLESHandle->meta();
#endif
	fail_backend();
}
