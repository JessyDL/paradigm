#pragma once
#include "defines.h"
#include <variant>

#ifdef PE_GLES
namespace core::igles
{
	class buffer;
}
#endif

namespace core::ivk
{
	class buffer;
}
namespace core::gfx
{
	using buffer = std::variant<
#ifdef PE_VULKAN
		core::ivk::buffer
#ifdef PE_GLES
		,
#endif
#endif
#ifdef PE_GLES
		core::igles::buffer
#endif
		>;
} // namespace core::gfx