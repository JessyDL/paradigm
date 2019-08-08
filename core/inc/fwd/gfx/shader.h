#pragma once
#include "defines.h"
#include <variant>

#ifdef PE_GLES
namespace core::igles
{
	class shader;
}
#endif

namespace core::ivk
{
	class shader;
}
namespace core::gfx
{
	using shader = std::variant<core::ivk::shader
#ifdef PE_GLES
		, core::igles::shader
	#endif
	>;
} // namespace core::gfx