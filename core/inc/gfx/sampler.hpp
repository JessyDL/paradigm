#pragma once
#include "fwd/gfx/sampler.hpp"
#include "resource/resource.hpp"
#include <variant>

namespace core::data {
class sampler_t;
}

namespace core::gfx {
class context;

class sampler_t {
  public:
#ifdef PE_VULKAN
	explicit sampler_t(core::resource::handle<core::ivk::sampler_t>& handle);
#endif
#ifdef PE_GLES
	explicit sampler_t(core::resource::handle<core::igles::sampler_t>& handle);
#endif

	sampler_t(core::resource::cache_t& cache,
			  const core::resource::metadata& metaData,
			  psl::meta::file* metaFile,
			  core::resource::handle<core::gfx::context> context,
			  core::resource::handle<core::data::sampler_t> sampler_data);
	~sampler_t() = default;

	sampler_t(const sampler_t& other)				 = delete;
	sampler_t(sampler_t&& other) noexcept			 = delete;
	sampler_t& operator=(const sampler_t& other)	 = delete;
	sampler_t& operator=(sampler_t&& other) noexcept = delete;

	template <core::gfx::graphics_backend backend>
	core::resource::handle<backend_type_t<sampler_t, backend>> resource() const noexcept {
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
	core::resource::handle<core::ivk::sampler_t> m_VKHandle;
#endif
#ifdef PE_GLES
	core::resource::handle<core::igles::sampler_t> m_GLESHandle;
#endif
};
}	 // namespace core::gfx
