#pragma once
#include "core/fwd/gfx/pipeline_cache.hpp"
#include "resource/resource.hpp"
#include <variant>

namespace core::gfx {
class context;

class pipeline_cache {
  public:
#ifdef PE_VULKAN
	explicit pipeline_cache(core::resource::handle<core::ivk::pipeline_cache>& handle);
#endif
#ifdef PE_GLES
	explicit pipeline_cache(core::resource::handle<core::igles::program_cache>& handle);
#endif


	pipeline_cache(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile,
				   core::resource::handle<core::gfx::context> context);
	~pipeline_cache() = default;

	pipeline_cache(const pipeline_cache& other)				   = default;
	pipeline_cache(pipeline_cache&& other) noexcept			   = default;
	pipeline_cache& operator=(const pipeline_cache& other)	   = default;
	pipeline_cache& operator=(pipeline_cache&& other) noexcept = default;

	template <core::gfx::graphics_backend backend>
	core::resource::handle<backend_type_t<pipeline_cache, backend>> resource() const noexcept {
#ifdef PE_VULKAN
		if constexpr(backend == graphics_backend::vulkan)
			return m_VKHandle;
#endif
#ifdef PE_GLES
		if constexpr(backend == graphics_backend::gles)
			return m_GLESHandle;
#endif
	};

  private:
	core::gfx::graphics_backend m_Backend {graphics_backend::undefined};
#ifdef PE_VULKAN
	core::resource::handle<core::ivk::pipeline_cache> m_VKHandle;
#endif
#ifdef PE_GLES
	core::resource::handle<core::igles::program_cache> m_GLESHandle;
#endif
};
}	 // namespace core::gfx
