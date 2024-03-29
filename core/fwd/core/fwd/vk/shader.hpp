#pragma once
#include "core/defines.hpp"
#include "core/fwd/resource/resource.hpp"


namespace core::ivk {
class shader;
}	 // namespace core::ivk

namespace core::meta {
class shader;
}
namespace core::resource {
template <>
struct resource_traits<core::ivk::shader> {
	using meta_type = core::meta::shader;
};
}	 // namespace core::resource
