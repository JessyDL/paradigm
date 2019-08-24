#pragma once
#include "defines.h"
#include "fwd/resource/resource.h"

namespace core::gfx
{
	class geometry;
} // namespace core::gfx

#ifdef PE_VULKAN
namespace core::ivk
{
	class geometry;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class geometry;
}
#endif
