#include "core/wgpu/drawpass.hpp"

#include "core/wgpu/context.hpp"
#include "core/wgpu/framebuffer.hpp"
#include "core/wgpu/swapchain.hpp"

using namespace core::resource;
using namespace core::iwgpu;

drawpass::drawpass(handle<context> context, handle<swapchain> swapchain) : m_Context(context), m_Swapchain(swapchain) {}
// drawpass::drawpass(handle<framebuffer> framebuffer) : m_Framebuffer(framebuffer) {}

void drawpass::clear() {
	m_DrawGroups.clear();
}
void drawpass::prepare() {}
bool drawpass::build(bool force) {
	return true;
}
void drawpass::present() {
	auto pass_descriptor = m_Swapchain->descriptor();

	auto command_encoder = m_Context->device().CreateCommandEncoder();
	auto render_pass	 = command_encoder.BeginRenderPass(&pass_descriptor);
	render_pass.End();
	auto command_buffer = command_encoder.Finish();
	m_Context->queue().Submit(1, &command_buffer);
	m_Swapchain->present();
}

bool drawpass::is_swapchain() const noexcept {
	return m_Swapchain;
}

void drawpass::add(core::gfx::drawgroup& group) noexcept {
	m_DrawGroups.push_back(group);
}

void drawpass::connect(psl::view_ptr<drawpass> pass) noexcept {}
void drawpass::connect(psl::view_ptr<computepass> pass) noexcept {}
void drawpass::disconnect(psl::view_ptr<drawpass> pass) noexcept {}
void drawpass::disconnect(psl::view_ptr<computepass> pass) noexcept {}
