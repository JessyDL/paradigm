#include "gfx/buffer.h"
#include "gfx/context.h"
#include "vk/context.h"
#include "gles/context.h"
#include "vk/buffer.h"
#include "gles/buffer.h"
#include "data/buffer.h"

using namespace core;
using namespace core::gfx;
using namespace core::resource;

buffer::buffer(const psl::UID& uid, cache& cache, psl::meta::file* meta, handle<context> context,
			   handle<data::buffer> data)
	: m_Handle(cache, uid, (meta)?meta->ID(): uid)
{
	switch(context->backend())
	{
	case graphics_backend::vulkan:
		m_Handle.load<core::ivk::buffer>(context->resource().get<core::ivk::context>(), data);
		break;
	case graphics_backend::gles: m_Handle.load<core::igles::buffer>(data); break;
	}
}

buffer::~buffer() {}