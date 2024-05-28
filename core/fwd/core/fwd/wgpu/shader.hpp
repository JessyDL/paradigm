#pragma once
#include "core/defines.hpp"
#include "core/fwd/resource/resource.hpp"


namespace core::iwgpu {
class shader;
}	 // namespace core::iwgpu

namespace core::meta {
class shader;
}
namespace core::resource {
template <>
struct resource_traits<core::iwgpu::shader> {
	using meta_type = core::meta::shader;
};
}	 // namespace core::resource
