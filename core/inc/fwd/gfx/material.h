#pragma once
#include "fwd/resource/resource.h"

namespace core::gfx
{
	class material;
} // namespace core::gfx

#ifdef PE_VULKAN
namespace core::ivk
{
	class material;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class material;
}
#endif