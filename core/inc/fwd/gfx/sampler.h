#pragma once
#include "defines.h"
#include <variant>

#ifdef PE_GLES
namespace core::igles
{
	class sampler;
}
#endif

namespace core::ivk
{
	class sampler;
}
namespace core::gfx
{
	using sampler = std::variant<core::ivk::sampler
#ifdef PE_GLES
								,
								 core::igles::sampler
#endif
								>;
} // namespace core::gfx