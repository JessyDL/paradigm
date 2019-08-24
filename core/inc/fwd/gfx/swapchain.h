#pragma once
#include "fwd/resource/resource.h"

namespace core::gfx
{
	class swapchain;
} // namespace core::gfx

#ifdef PE_VULKAN
namespace core::ivk
{
	class swapchain;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class swapchain;
}
#endif
