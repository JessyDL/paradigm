#include "core/gfx/geometry.hpp"
#include "core/gfx/buffer.hpp"
#include "core/gfx/context.hpp"

#ifdef PE_GLES
	#include "core/gles/geometry.hpp"
#endif

#ifdef PE_VULKAN
	#include "core/vk/geometry.hpp"
#endif
using namespace core;
using namespace core::gfx;
using namespace core::resource;

#ifdef PE_VULKAN
geometry_t::geometry_t(core::resource::handle<core::ivk::geometry_t>& handle)
	: m_Backend(graphics_backend::vulkan), m_VKHandle(handle) {}
#endif
#ifdef PE_GLES
geometry_t::geometry_t(core::resource::handle<core::igles::geometry_t>& handle)
	: m_Backend(graphics_backend::gles), m_GLESHandle(handle) {}
#endif

geometry_t::geometry_t(core::resource::cache_t& cache,
					   const core::resource::metadata& metaData,
					   psl::meta::file* metaFile,
					   core::resource::handle<context> context,
					   core::resource::handle<core::data::geometry_t> data,
					   core::resource::handle<buffer_t> geometryBuffer,
					   core::resource::handle<buffer_t> indicesBuffer)
	: m_Backend(context->backend()) {
	switch(m_Backend) {
#ifdef PE_GLES
	case graphics_backend::gles:
		m_GLESHandle = cache.create_using<core::igles::geometry_t>(metaData.uid,
																   data,
																   geometryBuffer->resource<graphics_backend::gles>(),
																   indicesBuffer->resource<graphics_backend::gles>());
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_VKHandle = cache.create_using<core::ivk::geometry_t>(metaData.uid,
															   context->resource<graphics_backend::vulkan>(),
															   data,
															   geometryBuffer->resource<graphics_backend::vulkan>(),
															   indicesBuffer->resource<graphics_backend::vulkan>());
		break;
#endif
	}
}

geometry_t::~geometry_t() {}

void geometry_t::recreate(core::resource::handle<core::data::geometry_t> data) {
#ifdef PE_GLES
	if(m_GLESHandle) {
		m_GLESHandle->recreate(data);
	}

#endif
#ifdef PE_VULKAN
	if(m_VKHandle) {
		m_VKHandle->recreate(data);
	}
#endif
}

void geometry_t::recreate(core::resource::handle<core::data::geometry_t> data,
						  core::resource::handle<core::gfx::buffer_t> geometryBuffer,
						  core::resource::handle<core::gfx::buffer_t> indicesBuffer) {
#ifdef PE_GLES
	if(m_GLESHandle && geometryBuffer->resource<graphics_backend::gles>() &&
	   indicesBuffer->resource<graphics_backend::gles>()) {
		m_GLESHandle->recreate(
		  data, geometryBuffer->resource<graphics_backend::gles>(), indicesBuffer->resource<graphics_backend::gles>());
	}

#endif
#ifdef PE_VULKAN
	if(m_VKHandle && geometryBuffer->resource<graphics_backend::vulkan>() &&
	   indicesBuffer->resource<graphics_backend::vulkan>()) {
		m_VKHandle->recreate(data,
							 geometryBuffer->resource<graphics_backend::vulkan>(),
							 indicesBuffer->resource<graphics_backend::vulkan>());
	}
#endif
}

size_t geometry_t::triangles() const noexcept {
#ifdef PE_GLES
	if(m_GLESHandle) {
		return m_GLESHandle->triangles();
	}
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) {
		return m_VKHandle->triangles();
	}
#endif
	return 0u;
}

size_t geometry_t::indices() const noexcept {
#ifdef PE_GLES
	if(m_GLESHandle) {
		return m_GLESHandle->indices();
	}
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) {
		return m_VKHandle->indices();
	}
#endif
	return 0u;
}

size_t geometry_t::vertices() const noexcept {
#ifdef PE_GLES
	if(m_GLESHandle) {
		return m_GLESHandle->vertices();
	}
#endif
#ifdef PE_VULKAN
	if(m_VKHandle) {
		return m_VKHandle->vertices();
	}
#endif
	return 0u;
}
