#pragma once
#include "core/fwd/resource/resource.hpp"

namespace core::iwgpu {
class texture_t;
}	 // namespace core::iwgpu

namespace core::meta {
class texture_t;
}
namespace core::resource {
template <>
struct resource_traits<core::iwgpu::texture_t> {
	using meta_type = core::meta::texture_t;
};
}	 // namespace core::resource
