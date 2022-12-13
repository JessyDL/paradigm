#pragma once
#include "fwd/resource/resource.hpp"

namespace core::ivk
{
class texture_t;
}	 // namespace core::ivk

namespace core::meta
{
class texture_t;
}
namespace core::resource
{
template <>
struct resource_traits<core::ivk::texture_t>
{
	using meta_type = core::meta::texture_t;
};
}	 // namespace core::resource
