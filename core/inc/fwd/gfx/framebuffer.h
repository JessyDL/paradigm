#pragma once
#include "fwd/resource/resource.h"
#include "gfx/types.h"

#ifdef PE_VULKAN
namespace core::ivk
{
	class framebuffer;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class framebuffer;
}
#endif

namespace core::gfx
{
	class framebuffer;

#ifdef PE_VULKAN

	template<>
	struct backend_type<framebuffer, graphics_backend::vulkan>
	{
		using type = core::ivk::framebuffer;
	};
#endif
#ifdef PE_GLES
	template<>
	struct backend_type<framebuffer, graphics_backend::gles>
	{
		using type = core::igles::framebuffer;
	};
#endif

} // namespace core::gfx

