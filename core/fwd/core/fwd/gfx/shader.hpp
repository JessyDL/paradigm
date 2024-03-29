#pragma once
#include "core/defines.hpp"
#include "core/fwd/resource/resource.hpp"
#include "core/gfx/types.hpp"

namespace core::meta {
class shader;
}

#ifdef PE_VULKAN
	#include "core/fwd/vk/shader.hpp"
#endif

#ifdef PE_GLES
	#include "core/fwd/gles/shader.hpp"
#endif

namespace core::gfx {
class shader;

#ifdef PE_VULKAN
template <>
struct backend_type<shader, graphics_backend::vulkan> {
	using type = core::ivk::shader;
};
#endif
#ifdef PE_GLES
template <>
struct backend_type<shader, graphics_backend::gles> {
	using type = core::igles::shader;
};
#endif
}	 // namespace core::gfx


namespace core::resource {
template <>
struct resource_traits<core::gfx::shader> {
	using meta_type = core::meta::shader;
};
}	 // namespace core::resource
