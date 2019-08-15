#include "gfx/shader.h"
#include "gfx/context.h"
#include "meta/shader.h"

#ifdef PE_VULKAN
#include "vk/shader.h"
#endif
#ifdef PE_GLES
#include "gles/shader.h"
#endif

using namespace core;
using namespace core::gfx;
using namespace core::resource;


shader::shader(core::resource::handle<value_type>& handle) : m_Handle(handle){};
shader::shader(core::resource::cache& cache, const core::resource::metadata& metaData, core::meta::shader* metaFile,
			   core::resource::handle<core::gfx::context> context)
{
	switch(context->backend())
	{
	case graphics_backend::gles: m_Handle << cache.create_using<core::igles::shader>(metaData.uid); break;
	case graphics_backend::vulkan:
		m_Handle << cache.create_using<core::ivk::shader>(metaData.uid, context->resource().get<core::ivk::context>());
		break;
	}
}


core::meta::shader* shader::meta() const noexcept
{
	if(m_Handle.contains<core::igles::shader>())
		return m_Handle.value<core::igles::shader>().meta();
	else
		return m_Handle.value<core::ivk::shader>().meta();

}