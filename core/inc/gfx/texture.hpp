#pragma once
#include "fwd/gfx/texture.hpp"
#include "resource/resource.hpp"

namespace core::gfx
{
class context;
class texture_t
{
  public:
	using meta_type = core::meta::texture_t;

#ifdef PE_VULKAN
	explicit texture_t(core::resource::handle<core::ivk::texture_t>& handle);
#endif
#ifdef PE_GLES
	explicit texture_t(core::resource::handle<core::igles::texture_t>& handle);
#endif

	texture_t(core::resource::cache_t& cache,
			  const core::resource::metadata& metaData,
			  core::meta::texture_t* metaFile,
			  core::resource::handle<core::gfx::context> context);

	~texture_t();

	texture_t(const texture_t& other)				 = delete;
	texture_t(texture_t&& other) noexcept			 = delete;
	texture_t& operator=(const texture_t& other)	 = delete;
	texture_t& operator=(texture_t&& other) noexcept = delete;

	template <core::gfx::graphics_backend backend>
	core::resource::handle<backend_type_t<texture_t, backend>> resource() const noexcept
	{
#ifdef PE_VULKAN
		if constexpr(backend == graphics_backend::vulkan) return m_VKHandle;
#endif
#ifdef PE_GLES
		if constexpr(backend == graphics_backend::gles) return m_GLESHandle;
#endif
	};

	const core::meta::texture_t& meta() const noexcept;

  private:
	core::gfx::graphics_backend m_Backend {graphics_backend::undefined};
#ifdef PE_VULKAN
	core::resource::handle<core::ivk::texture_t> m_VKHandle;
#endif
#ifdef PE_GLES
	core::resource::handle<core::igles::texture_t> m_GLESHandle;
#endif
};
}	 // namespace core::gfx