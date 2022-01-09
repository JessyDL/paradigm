#pragma once
#include "defines.h"
#include "fwd/resource/resource.h"


namespace core::ivk
{
	class shader;
}	 // namespace core::ivk

namespace core::meta
{
	class shader;
}
namespace core::resource
{
	template <>
	struct resource_traits<core::ivk::shader>
	{
		using meta_type = core::meta::shader;
	};
}	 // namespace core::resource