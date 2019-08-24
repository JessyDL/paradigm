#pragma once
#include "resource/resource.hpp"
#include <variant>
#include "psl/view_ptr.h"
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
	class drawgroup;

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
		~pass();

		pass(const pass& other)		= delete;
		pass(pass&& other) noexcept = delete;
		pass& operator=(const pass& other) = delete;
		pass& operator=(pass&& other) noexcept = delete;

		bool is_swapchain() const noexcept;

		
		void clear();
		void prepare();
		bool build();
		void present();

		bool connect(psl::view_ptr<core::gfx::pass> child) noexcept;
		bool disconnect(psl::view_ptr<core::gfx::pass> child) noexcept;
		void add(core::gfx::drawgroup& group) noexcept;

		value_type resource() const noexcept { return m_Handle; };

	  private:
		value_type m_Handle;
	};
} // namespace core::gfx