#pragma once
#include "defines.h"
#include "fwd/resource/resource.h"
namespace core::gfx
{
	class sampler;
} // namespace core::gfx

#ifdef PE_VULKAN
namespace core::ivk
{
	class sampler;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class sampler;
}
#endif
