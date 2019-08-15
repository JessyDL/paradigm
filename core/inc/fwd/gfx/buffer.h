#pragma once
#include "defines.h"
#include "fwd/resource/resource.h"

namespace core::gfx
{
	class buffer;
} // namespace core::gfx

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