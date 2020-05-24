#pragma once
#include "defines.h"
#include "fwd/resource/resource.h"
#include "gfx/types.h"

#ifdef PE_VULKAN
namespace core::ivk
{
	class buffer;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class buffer;
}
#endif

namespace core::gfx
{
	class buffer;
	struct shader_buffer_binding;

#ifdef PE_VULKAN
	template<>
	struct backend_type<buffer, graphics_backend::vulkan>
	{
		using type = core::ivk::buffer;
	};
#endif
#ifdef PE_GLES
	template<>
	struct backend_type<buffer, graphics_backend::gles>
	{
		using type = core::igles::buffer;
	};
#endif
} // namespace core::gfx
