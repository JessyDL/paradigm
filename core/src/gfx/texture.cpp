#include "gfx/texture.h"
#include "gfx/context.h"
#include "meta/texture.h"

#ifdef PE_VULKAN
#include "vk/texture.h"
#endif
#ifdef PE_GLES
#include "gles/texture.h"
#endif

using namespace core;
using namespace core::gfx;
using namespace core::resource;

texture::texture(core::resource::handle<value_type>& handle) : m_Handle(handle){};
texture::texture(core::resource::cache& cache, const core::resource::metadata& metaData,
				 core::meta::texture* metaFile,
				 core::resource::handle<core::gfx::context> context)
{
	switch (context->backend())
	{
#ifdef PE_VULKAN
	case graphics_backend::vulkan:
		m_Handle << cache.create_using<core::ivk::texture>(metaData.uid, context->resource().get<core::ivk::context>());
		break;
#endif
#ifdef PE_GLES
	case graphics_backend::gles: m_Handle << cache.create_using<core::igles::texture>(metaData.uid); break;
#endif
	}
}

texture::~texture() {}