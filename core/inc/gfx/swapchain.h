#pragma once
#include "fwd/gfx/swapchain.h"
#include "resource/resource.hpp"
#include <variant>

namespace core::os
{
	class surface;
}

namespace core::gfx
{
	class context;

	class swapchain
	{
	  public:
#ifdef PE_VULKAN
		explicit swapchain(core::resource::handle<core::ivk::swapchain>& handle);
#endif
#ifdef PE_GLES
		explicit swapchain(core::resource::handle<core::igles::swapchain>& handle);
#endif

		swapchain(core::resource::cache& cache,
				  const core::resource::metadata& metaData,
				  psl::meta::file* metaFile,
				  core::resource::handle<core::os::surface> surface,
				  core::resource::handle<core::gfx::context> context,
				  bool use_depth = true);

		~swapchain() {};

		swapchain(const swapchain& other)	  = delete;
		swapchain(swapchain&& other) noexcept = delete;
		swapchain& operator=(const swapchain& other) = delete;
		swapchain& operator=(swapchain&& other) noexcept = delete;

		template <core::gfx::graphics_backend backend>
		core::resource::handle<backend_type_t<swapchain, backend>> resource() const noexcept
		{
#ifdef PE_VULKAN
			if constexpr(backend == graphics_backend::vulkan) return m_VKHandle;
#endif
#ifdef PE_GLES
			if constexpr(backend == graphics_backend::gles) return m_GLESHandle;
#endif
		};

	  private:
		core::gfx::graphics_backend m_Backend {graphics_backend::undefined};
#ifdef PE_VULKAN
		core::resource::handle<core::ivk::swapchain> m_VKHandle;
#endif
#ifdef PE_GLES
		core::resource::handle<core::igles::swapchain> m_GLESHandle;
#endif
	};
}	 // namespace core::gfx