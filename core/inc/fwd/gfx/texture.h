#pragma once
#include "defines.h"
#include <variant>

#ifdef PE_GLES
namespace core::igles
{
	class texture;
}
#endif

namespace core::ivk
{
	class texture;
}
namespace core::gfx
{
	using texture = std::variant<core::ivk::texture
#ifdef PE_GLES
								 ,
								 core::igles::texture
#endif
								 >;
} // namespace core::gfx