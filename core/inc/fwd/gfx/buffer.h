#pragma once
#include "defines.h"
#include "fwd/resource/resource.h"
#include "gfx/types.h"

#ifdef PE_VULKAN
namespace core::ivk
{
	class buffer_t;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class buffer_t;
}
#endif

namespace core::gfx
{
	class buffer_t;
	struct shader_buffer_binding;

#ifdef PE_VULKAN
	template <>
	struct backend_type<buffer_t, graphics_backend::vulkan>
	{
		using type = core::ivk::buffer_t;
	};
#endif
#ifdef PE_GLES
	template <>
	struct backend_type<buffer_t, graphics_backend::gles>
	{
		using type = core::igles::buffer_t;
	};
#endif
}	 // namespace core::gfx
