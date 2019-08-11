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


shader::shader(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile,
			   core::resource::handle<core::gfx::context> context)
	: m_Handle(cache, uid, (metaFile)?metaFile->ID():uid)
{
	switch(context->backend())
	{
	case graphics_backend::gles: m_Handle.load<core::igles::shader>(); break;
	case graphics_backend::vulkan:
		m_Handle.load<core::ivk::shader>(context->resource().get<core::ivk::context>());
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