#pragma once
#include "defines.h"
#include <variant>

#ifdef PE_GLES
namespace core::igles
{
	class context;
}
#endif

namespace core::ivk
{
	class context;
}
namespace core::gfx
{
	using context = std::variant<core::ivk::context
#ifdef PE_GLES
								 ,
								 core::igles::context
#endif
								 >;
} // namespace core::gfx