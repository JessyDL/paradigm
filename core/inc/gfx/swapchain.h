#pragma once
#include "resource/resource.hpp"

#include <variant>

namespace core::os
{
	class surface;
}

namespace core::ivk
{
	class swapchain;
}
namespace core::igles
{
	class swapchain;
}

namespace core::gfx
{
	class context;

	class swapchain
	{
		using value_type = std::variant<
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
	  public:
		swapchain(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile, core::resource::handle<core::os::surface> surface,
				  core::resource::handle<core::gfx::context> context, bool use_depth = true);

		~swapchain() = default;

		swapchain(const swapchain& other)	 = delete;
		swapchain(swapchain&& other) noexcept = delete;
		swapchain& operator=(const swapchain& other) = delete;
		swapchain& operator=(swapchain&& other) noexcept = delete;

		core::resource::handle<value_type> resource() noexcept { return m_Handle; };

	  private:
		core::resource::handle<value_type> m_Handle;
	};
} // namespace core::gfx