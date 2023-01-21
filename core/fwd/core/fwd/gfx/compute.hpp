#pragma once
#include "core/defines.hpp"
#include "core/fwd/resource/resource.hpp"


namespace core::gfx {
class compute;
}	 // namespace core::gfx

namespace core::meta {
class shader;
}

#ifdef PE_VULKAN
	#include "core/fwd/vk/compute.hpp"
#endif

#ifdef PE_GLES
	#include "core/fwd/gles/compute.hpp"
#endif

namespace core::resource {
template <>
struct resource_traits<core::gfx::compute> {
	using meta_type = core::meta::shader;
};
}	 // namespace core::resource
