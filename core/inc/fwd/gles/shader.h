#pragma once
#include "defines.h"
#include "fwd/resource/resource.h"


namespace core::igles
{
	class shader;
}	 // namespace core::igles

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
}	 // namespace core::resource