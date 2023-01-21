#pragma once
#include "core/fwd/gfx/shader.hpp"
#include "resource/resource.hpp"

namespace core::gfx {
class context;

class shader {
	friend class core::resource::cache_t;

  public:
#ifdef PE_VULKAN
	explicit shader(core::resource::handle<core::ivk::shader>& handle);
#endif
#ifdef PE_GLES
	explicit shader(core::resource::handle<core::igles::shader>& handle);
#endif

	shader(core::resource::cache_t& cache,
		   const core::resource::metadata& metaData,
		   core::meta::shader* metaFile,
		   core::resource::handle<core::gfx::context> context);
	~shader() = default;

	shader(const shader& other)				   = default;
	shader(shader&& other) noexcept			   = default;
	shader& operator=(const shader& other)	   = default;
	shader& operator=(shader&& other) noexcept = default;


	template <core::gfx::graphics_backend backend>
	core::resource::handle<backend_type_t<shader, backend>> resource() const noexcept {
#ifdef PE_VULKAN
		if constexpr(backend == graphics_backend::vulkan)
			return m_VKHandle;
#endif
#ifdef PE_GLES
		if constexpr(backend == graphics_backend::gles)
			return m_GLESHandle;
#endif
	};

	core::meta::shader* meta() const noexcept;

  private:
	core::gfx::graphics_backend m_Backend {graphics_backend::undefined};
#ifdef PE_VULKAN
	core::resource::handle<core::ivk::shader> m_VKHandle;
#endif
#ifdef PE_GLES
	core::resource::handle<core::igles::shader> m_GLESHandle;
#endif
};
}	 // namespace core::gfx
