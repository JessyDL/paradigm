#pragma once
#include "fwd/resource/resource.hpp"
#include "gfx/types.hpp"

#ifdef PE_VULKAN
namespace core::ivk {
class context;
}
#endif

#ifdef PE_GLES
namespace core::igles {
class context;
}
#endif
namespace core::gfx {
class context;

#ifdef PE_VULKAN
template <>
struct backend_type<context, graphics_backend::vulkan> {
	using type = core::ivk::context;
};
#endif
#ifdef PE_GLES
template <>
struct backend_type<context, graphics_backend::gles> {
	using type = core::igles::context;
};
#endif
}	 // namespace core::gfx
