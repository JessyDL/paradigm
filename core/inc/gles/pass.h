#pragma once
#include "resource/resource.hpp"
#include "gfx/drawgroup.h"

namespace core::igles
{
	class swapchain;
	class framebuffer;

	class pass
	{
	  public:
		pass(core::resource::handle<swapchain> swapchain);
		pass(core::resource::handle<framebuffer> framebuffer);
		~pass() = default;

		pass(const pass& other)		= default;
		pass(pass&& other) noexcept = default;
		pass& operator=(const pass& other) = default;
		pass& operator=(pass&& other) noexcept = default;

		void clear();
		void prepare();
		bool build();
		void present();

		bool is_swapchain() const noexcept { return true; }
		void add(core::gfx::drawgroup& group) noexcept;

	  private:
		core::resource::handle<swapchain> m_Swapchain;
		core::resource::handle<framebuffer> m_Framebuffer;
		psl::array<core::gfx::drawgroup> m_DrawGroups;
	};
} // namespace core::igles