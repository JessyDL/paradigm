#pragma once
#include "resource/resource.hpp"
#include <variant>

#ifdef PE_GLES
namespace core::igles
{
	class pass;
}
#endif
#ifdef PE_VULKAN
namespace core::ivk
{
	class pass;
}
#endif

namespace core::gfx
{
	class context;
	class framebuffer;
	class swapchain;

	class pass
	{
		using value_type = std::variant<
#ifdef PE_VULKAN
			core::ivk::pass*
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::pass*
#endif
			>;
	  public:
		pass(core::resource::handle<context> context, core::resource::handle<framebuffer> framebuffer);
		pass(core::resource::handle<context> context, core::resource::handle<swapchain> swapchain);
		~pass() = default;

		pass(const pass& other)		= delete;
		pass(pass&& other) noexcept = delete;
		pass& operator=(const pass& other) = delete;
		pass& operator=(pass&& other) noexcept = delete;

		value_type resource() const noexcept { return m_Handle; };

	  private:
		value_type m_Handle;
	};
} // namespace core::gfx