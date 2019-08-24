#include "gfx/geometry.h"
#include "gfx/context.h"
#include "gfx/buffer.h"

#ifdef PE_GLES
#include "gles/geometry.h"
#endif

#ifdef PE_VULKAN
#include "vk/geometry.h"
#endif
using namespace core;
using namespace core::gfx;
using namespace core::resource;

geometry::geometry(core::resource::handle<value_type>& handle) : m_Handle(handle){};
geometry::geometry(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				   core::resource::handle<context> context,
				   core::resource::handle<core::data::geometry> data, core::resource::handle<buffer> geometryBuffer,
				   core::resource::handle<buffer> indicesBuffer)
{
	switch(context->backend())
	{
	case graphics_backend::gles:
		m_Handle << cache.create_using<core::igles::geometry>(metaData.uid, data,
															  geometryBuffer->resource().get<core::igles::buffer>(),
											 indicesBuffer->resource().get<core::igles::buffer>());
		break;
	case graphics_backend::vulkan:
		m_Handle << cache.create_using<core::ivk::geometry>(metaData.uid, context->resource().get<core::ivk::context>(),
															data,
										   geometryBuffer->resource().get<core::ivk::buffer>(),
										   indicesBuffer->resource().get<core::ivk::buffer>());
		break;
	}
}

geometry::~geometry() {}