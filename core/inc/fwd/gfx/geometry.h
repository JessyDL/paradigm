#pragma once
#include "defines.h"
#include <variant>

#ifdef PE_GLES
namespace core::igles
{
	class geometry;
}
#endif

namespace core::ivk
{
	class geometry;
}
namespace core::gfx
{
	using geometry = std::variant<
#ifdef PE_VULKAN
		core::ivk::geometry
#ifdef PE_GLES
		,
#endif
#endif
#ifdef PE_GLES
		core::igles::geometry
#endif
		>;
} // namespace core::gfx