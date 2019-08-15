#pragma once
#include "resource/resource.hpp"
#include "fwd/gfx/swapchain.h"
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
		using alias_type = core::resource::alias<
#ifdef PE_VULKAN
			core::ivk::swapchain
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::swapchain
#endif
			>;
		using value_type = alias_type;
		swapchain(core::resource::handle<value_type>& handle);
		swapchain(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				  core::resource::handle<core::os::surface> surface,
				  core::resource::handle<core::gfx::context> context, bool use_depth = true);

		~swapchain(){};

		swapchain(const swapchain& other)	 = delete;
		swapchain(swapchain&& other) noexcept = delete;
		swapchain& operator=(const swapchain& other) = delete;
		swapchain& operator=(swapchain&& other) noexcept = delete;

		core::resource::handle<value_type> resource() noexcept { return m_Handle; };

	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx