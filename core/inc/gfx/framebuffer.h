#pragma once
#include "fwd/gfx/framebuffer.h"
#include "resource/resource.hpp"

namespace core::data
{
	class framebuffer;
}

namespace core::gfx
{
	class context;
	class texture;

	class framebuffer
	{
	  public:
#ifdef PE_VULKAN
		explicit framebuffer(core::resource::handle<core::ivk::framebuffer>& handle);
#endif
#ifdef PE_GLES
		explicit framebuffer(core::resource::handle<core::igles::framebuffer>& handle);
#endif

		framebuffer(core::resource::cache& cache,
					const core::resource::metadata& metaData,
					psl::meta::file* metaFile,
					core::resource::handle<core::gfx::context> context,
					core::resource::handle<data::framebuffer> data);
		~framebuffer() = default;

		framebuffer(const framebuffer& other)	  = delete;
		framebuffer(framebuffer&& other) noexcept = delete;
		framebuffer& operator=(const framebuffer& other) = delete;
		framebuffer& operator=(framebuffer&& other) noexcept = delete;

		template <core::gfx::graphics_backend backend>
		core::resource::handle<backend_type_t<framebuffer, backend>> resource() const noexcept
		{
#ifdef PE_VULKAN
			if constexpr(backend == graphics_backend::vulkan) return m_VKHandle;
#endif
#ifdef PE_GLES
			if constexpr(backend == graphics_backend::gles) return m_GLESHandle;
#endif
		};

		texture texture(size_t index) const noexcept;

	  private:
		core::gfx::graphics_backend m_Backend {0};
#ifdef PE_VULKAN
		core::resource::handle<core::ivk::framebuffer> m_VKHandle;
#endif
#ifdef PE_GLES
		core::resource::handle<core::igles::framebuffer> m_GLESHandle;
#endif
	};
}	 // namespace core::gfx