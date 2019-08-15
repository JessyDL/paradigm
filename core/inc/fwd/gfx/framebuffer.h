#pragma once
#include "fwd/resource/resource.h"

namespace core::gfx
{
	class framebuffer;
} // namespace core::gfx

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