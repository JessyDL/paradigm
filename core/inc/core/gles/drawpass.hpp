#pragma once
#include "core/gfx/computecall.hpp"
#include "core/gfx/drawgroup.hpp"
#include "core/gles/types.hpp"
#include "core/resource/resource.hpp"

namespace core::igles {
class swapchain;
class framebuffer_t;
class computepass;

class drawpass {
	struct memory_barrier_t {
		GLbitfield barrier {0};
		uint32_t usage {0};
	};

  public:
	drawpass(core::resource::handle<swapchain> swapchain);
	drawpass(core::resource::handle<framebuffer_t> framebuffer);
	~drawpass() = default;

	drawpass(const drawpass& other)				   = default;
	drawpass(drawpass&& other) noexcept			   = default;
	drawpass& operator=(const drawpass& other)	   = default;
	drawpass& operator=(drawpass&& other) noexcept = default;

	void clear();
	void prepare();
	bool build();
	void present();

	bool is_swapchain() const noexcept { return m_Swapchain; }
	void add(core::gfx::drawgroup& group) noexcept;

	void connect(psl::view_ptr<drawpass> pass) noexcept;
	void connect(psl::view_ptr<computepass> pass) noexcept;
	void disconnect(psl::view_ptr<drawpass> pass) noexcept;
	void disconnect(psl::view_ptr<computepass> pass) noexcept;

  private:
	core::resource::handle<swapchain> m_Swapchain {};
	core::resource::handle<framebuffer_t> m_Framebuffer {};
	psl::array<memory_barrier_t> m_MemoryBarriers {};
	psl::array<core::gfx::drawgroup> m_DrawGroups {};
};
}	 // namespace core::igles
