#pragma once
#include "fwd/resource/resource.hpp"

namespace core::igles
{
class texture_t;
}	 // namespace core::igles

namespace core::meta
{
class texture_t;
}
namespace core::resource
{
template <>
struct resource_traits<core::igles::texture_t>
{
	using meta_type = core::meta::texture_t;
};
}	 // namespace core::resource