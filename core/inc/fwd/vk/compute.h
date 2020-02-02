#pragma once
#include "fwd/resource/resource.h"
#include "defines.h"


namespace core::ivk
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
	struct resource_traits<core::ivk::compute>
	{
		using meta_type = core::meta::shader;
	};
} // namespace core::resource