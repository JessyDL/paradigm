#pragma once
#include "fwd/resource/resource.h"
#include "defines.h"


namespace core::igles
{
	class compute;
} // namespace core::igles

namespace core::meta
{
	class shader;
}
namespace core::resource
{
	template <>
	struct resource_traits<core::igles::compute>
	{
		using meta_type = core::meta::shader;
	};
} // namespace core::resource