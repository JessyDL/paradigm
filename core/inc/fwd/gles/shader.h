#pragma once
#include "fwd/resource/resource.h"
#include "defines.h"


namespace core::igles
{
	class shader;
} // namespace core::gfx

namespace core::meta
{
	class shader;
}
namespace core::resource
{
	template <>
	struct resource_traits<core::igles::shader>
	{
		using meta_type = core::meta::shader;
	};
} // namespace core::resource