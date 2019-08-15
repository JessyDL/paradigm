#pragma once
#include "fwd/resource/resource.h"

namespace core::gfx
{
	class pipeline_cache;
} // namespace core::gfx

#ifdef PE_VULKAN
namespace core::ivk
{
	class pipeline_cache;
}
#endif

#ifdef PE_GLES
namespace core::igles
{
	class program_cache;
}
#endif
