#pragma once
#include "core/gfx/drawgroup.hpp"
#include "core/gfx/types.hpp"
#include "core/resource/resource.hpp"

namespace core::iwgpu {
class context;
class swapchain;
class framebuffer;
class computepass;

class drawpass {
  public:
	drawpass(core::resource::handle<context> context, core::resource::handle<swapchain> swapchain);
	// drawpass(core::resource::handle<context> context, core::resource::handle<framebuffer> framebuffer);
	~drawpass() = default;

	drawpass(const drawpass& other)		= default;
	drawpass(drawpass&& other) noexcept = default;

	drawpass& operator=(const drawpass& other)	   = default;
	drawpass& operator=(drawpass&& other) noexcept = default;

	void clear();
	void prepare();
	bool build(bool force = false);
	void present();

	bool is_swapchain() const noexcept;

	void add(core::gfx::drawgroup& group) noexcept;

	void connect(psl::view_ptr<drawpass> pass) noexcept;
	void connect(psl::view_ptr<computepass> pass) noexcept;
	void disconnect(psl::view_ptr<drawpass> pass) noexcept;
	void disconnect(psl::view_ptr<computepass> pass) noexcept;

  private:
	core::resource::handle<context> m_Context {};
	core::resource::handle<swapchain> m_Swapchain {};
	// core::resource::handle<framebuffer> m_Framebuffer {};
	psl::array<core::gfx::drawgroup> m_DrawGroups {};
};
}	 // namespace core::iwgpu
