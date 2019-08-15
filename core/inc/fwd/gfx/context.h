#pragma once
#include "fwd/resource/resource.h"

namespace core::gfx
{
	class context;
} // namespace core::gfx

#ifdef PE_VULKAN
namespace core::ivk
{
	class context;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class context;
}
#endif