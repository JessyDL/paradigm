#pragma once
#include "core/fwd/resource/resource.hpp"
#include "gfx/types.hpp"

#ifdef PE_VULKAN
namespace core::ivk {
class pipeline_cache;
}
#endif

#ifdef PE_GLES
namespace core::igles {
class program_cache;
}
#endif

namespace core::gfx {
class pipeline_cache;

#ifdef PE_VULKAN
template <>
struct backend_type<pipeline_cache, graphics_backend::vulkan> {
	using type = core::ivk::pipeline_cache;
};
#endif
#ifdef PE_GLES
template <>
struct backend_type<pipeline_cache, graphics_backend::gles> {
	using type = core::igles::program_cache;
};
#endif
}	 // namespace core::gfx
