#pragma once
#include "core/fwd/gfx/geometry.hpp"
#include "core/resource/resource.hpp"

namespace core::data {
class geometry_t;
}

namespace core::gfx {
class context;
class buffer_t;

class geometry_t {
  public:
#ifdef PE_VULKAN
	explicit geometry_t(core::resource::handle<core::ivk::geometry_t>& handle);
#endif
#ifdef PE_GLES
	explicit geometry_t(core::resource::handle<core::igles::geometry_t>& handle);
#endif

	geometry_t(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile,
			   core::resource::handle<context> context,
			   core::resource::handle<core::data::geometry_t> data,
			   core::resource::handle<buffer_t> geometryBuffer,
			   core::resource::handle<buffer_t> indicesBuffer);
	~geometry_t();
	geometry_t(const geometry_t&)			 = delete;
	geometry_t(geometry_t&&)				 = delete;
	geometry_t& operator=(const geometry_t&) = delete;
	geometry_t& operator=(geometry_t&&)		 = delete;


	void recreate(core::resource::handle<core::data::geometry_t> data);
	void recreate(core::resource::handle<core::data::geometry_t> data,
				  core::resource::handle<core::gfx::buffer_t> geometryBuffer,
				  core::resource::handle<core::gfx::buffer_t> indicesBuffer);
	template <core::gfx::graphics_backend backend>
	core::resource::handle<backend_type_t<geometry_t, backend>> resource() const noexcept {
#ifdef PE_VULKAN
		if constexpr(backend == graphics_backend::vulkan)
			return m_VKHandle;
#endif
#ifdef PE_GLES
		if constexpr(backend == graphics_backend::gles)
			return m_GLESHandle;
#endif
	};

	size_t vertices() const noexcept;
	size_t indices() const noexcept;
	size_t triangles() const noexcept;

  private:
	core::gfx::graphics_backend m_Backend {graphics_backend::undefined};
#ifdef PE_VULKAN
	core::resource::handle<core::ivk::geometry_t> m_VKHandle;
#endif
#ifdef PE_GLES
	core::resource::handle<core::igles::geometry_t> m_GLESHandle;
#endif
};
}	 // namespace core::gfx
