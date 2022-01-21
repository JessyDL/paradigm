#pragma once
#include "fwd/resource/resource.hpp"
#include "defines.hpp"


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