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
				   core::resource::handle<context> context, core::resource::handle<core::data::geometry> data,
				   core::resource::handle<buffer> geometryBuffer, core::resource::handle<buffer> indicesBuffer)
{
	switch(context->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_Handle << cache.create_using<core::igles::geometry>(metaData.uid, data,
															  geometryBuffer->resource().get<core::igles::buffer>(),
															  indicesBuffer->resource().get<core::igles::buffer>());
		break;
#endif
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_Handle << cache.create_using<core::ivk::geometry>(metaData.uid, context->resource().get<core::ivk::context>(),
															data, geometryBuffer->resource().get<core::ivk::buffer>(),
															indicesBuffer->resource().get<core::ivk::buffer>());
		break;
#endif
	}
}

geometry::~geometry() {}

void geometry::recreate(core::resource::handle<core::data::geometry> data)
{
	if (m_Handle.contains<igles::geometry>())
	{
		m_Handle.value<igles::geometry>().recreate(data);
	}

	if (m_Handle.contains<ivk::geometry>())
	{
		m_Handle.value<ivk::geometry>().recreate(data);
	}
}

void geometry::recreate(core::resource::handle<core::data::geometry> data,
						core::resource::handle<core::gfx::buffer> geometryBuffer,
						core::resource::handle<core::gfx::buffer> indicesBuffer)
{
	if(m_Handle.contains<igles::geometry>() && geometryBuffer->resource().contains<igles::buffer>() &&
	   indicesBuffer->resource().contains<igles::buffer>())
	{
		m_Handle.value<igles::geometry>().recreate(data, geometryBuffer->resource().get<core::igles::buffer>(),
												   indicesBuffer->resource().get<core::igles::buffer>());
	}

	if(m_Handle.contains<ivk::geometry>() && geometryBuffer->resource().contains<ivk::buffer>() &&
	   indicesBuffer->resource().contains<ivk::buffer>())
	{
		m_Handle.value<ivk::geometry>().recreate(data, geometryBuffer->resource().get<core::ivk::buffer>(),
												 indicesBuffer->resource().get<core::ivk::buffer>());
	}
}