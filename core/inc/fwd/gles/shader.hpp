#pragma once
#include "defines.hpp"
#include "fwd/resource/resource.hpp"


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