#pragma once
#include "fwd/resource/resource.h"

namespace core::ivk
{
	class texture;
} // namespace core::ivk

namespace core::meta
{
	class texture;
}
namespace core::resource
{
	template <>
	struct resource_traits<core::ivk::texture>
	{
		using meta_type = core::meta::texture;
	};
} // namespace core::resource