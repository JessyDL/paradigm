#pragma once
#include "fwd/gfx/texture.h"
#include "resource/resource.hpp"

namespace core::gfx
{
	class context;
	class texture
	{
	  public:
		using meta_type = core::meta::texture;

#ifdef PE_VULKAN
		explicit texture(core::resource::handle<core::ivk::texture>& handle);
#endif
#ifdef PE_GLES
		explicit texture(core::resource::handle<core::igles::texture>& handle);
#endif

		texture(core::resource::cache& cache, const core::resource::metadata& metaData, core::meta::texture* metaFile,
				core::resource::handle<core::gfx::context> context);

		~texture();

		texture(const texture& other)	  = delete;
		texture(texture&& other) noexcept = delete;
		texture& operator=(const texture& other) = delete;
		texture& operator=(texture&& other) noexcept = delete;

		template <core::gfx::graphics_backend backend>
		core::resource::handle<backend_type_t<texture, backend>> resource() const noexcept
		{
#ifdef PE_VULKAN
			if constexpr(backend == graphics_backend::vulkan) return m_VKHandle;
#endif
#ifdef PE_GLES
			if constexpr(backend == graphics_backend::gles) return m_GLESHandle;
#endif
		};

		const core::meta::texture& meta() const noexcept;

	  private:
		core::gfx::graphics_backend m_Backend{graphics_backend::undefined};
#ifdef PE_VULKAN
		core::resource::handle<core::ivk::texture> m_VKHandle;
#endif
#ifdef PE_GLES
		core::resource::handle<core::igles::texture> m_GLESHandle;
#endif
	};
} // namespace core::gfx