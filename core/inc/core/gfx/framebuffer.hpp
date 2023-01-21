#pragma once
#include "core/fwd/gfx/framebuffer.hpp"
#include "core/resource/resource.hpp"

namespace core::data {
class framebuffer_t;
}

namespace core::gfx {
class context;
class texture_t;

class framebuffer_t {
  public:
#ifdef PE_VULKAN
	explicit framebuffer_t(core::resource::handle<core::ivk::framebuffer_t>& handle);
#endif
#ifdef PE_GLES
	explicit framebuffer_t(core::resource::handle<core::igles::framebuffer_t>& handle);
#endif

	framebuffer_t(core::resource::cache_t& cache,
				  const core::resource::metadata& metaData,
				  psl::meta::file* metaFile,
				  core::resource::handle<core::gfx::context> context,
				  core::resource::handle<data::framebuffer_t> data);
	~framebuffer_t() = default;

	framebuffer_t(const framebuffer_t& other)				 = delete;
	framebuffer_t(framebuffer_t&& other) noexcept			 = delete;
	framebuffer_t& operator=(const framebuffer_t& other)	 = delete;
	framebuffer_t& operator=(framebuffer_t&& other) noexcept = delete;

	template <core::gfx::graphics_backend backend>
	core::resource::handle<backend_type_t<framebuffer_t, backend>> resource() const noexcept {
#ifdef PE_VULKAN
		if constexpr(backend == graphics_backend::vulkan)
			return m_VKHandle;
#endif
#ifdef PE_GLES
		if constexpr(backend == graphics_backend::gles)
			return m_GLESHandle;
#endif
	};

	texture_t texture(size_t index) const noexcept;

  private:
	core::gfx::graphics_backend m_Backend {0};
#ifdef PE_VULKAN
	core::resource::handle<core::ivk::framebuffer_t> m_VKHandle;
#endif
#ifdef PE_GLES
	core::resource::handle<core::igles::framebuffer_t> m_GLESHandle;
#endif
};
}	 // namespace core::gfx
