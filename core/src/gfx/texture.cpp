#include "gfx/texture.h"
#include "gfx/context.h"

#ifdef PE_VULKAN
#include "vk/texture.h"
#endif
#ifdef PE_GLES
#include "gles/texture.h"
#endif

using namespace core;
using namespace core::gfx;
using namespace core::resource;

texture::texture(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile,
				 core::resource::handle<core::gfx::context> context)
	: m_Handle(cache, uid, metaFile->ID())
{
	switch (context->backend())
	{
	case graphics_backend::vulkan:
		m_Handle.load<core::ivk::texture>(context->resource().get<core::ivk::context>());
		break;
	case graphics_backend::gles: m_Handle.load<core::igles::texture>(); break;
	}
}

texture::~texture() {}