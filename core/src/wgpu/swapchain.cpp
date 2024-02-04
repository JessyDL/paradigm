#include "core/wgpu/swapchain.hpp"

#include "core/os/surface.hpp"
#include "core/wgpu/context.hpp"

#include "core/resource/cache.hpp"

using namespace core::iwgpu;

swapchain::swapchain(core::resource::cache_t& cache,
					 const core::resource::metadata& metaData,
					 psl::meta::file* metaFile,
					 core::resource::handle<core::os::surface> surface,
					 core::resource::handle<core::iwgpu::context> context,
					 bool use_depth)
	: m_UseDepth(use_depth) {
	auto swap_chain_desc		= wgpu::SwapChainDescriptor();
	swap_chain_desc.usage		= wgpu::TextureUsage::RenderAttachment;
	swap_chain_desc.format		= context->surface().GetPreferredFormat(context->adapter());
	swap_chain_desc.width		= surface->data().width();
	swap_chain_desc.height		= surface->data().height();
	swap_chain_desc.presentMode = wgpu::PresentMode::Fifo;	  // todo make this configurable

	m_SwapChain = context->device().CreateSwapChain(context->surface(), &swap_chain_desc);
}
bool swapchain::present() {
	m_SwapChain.Present();
	return true;
}

auto swapchain::descriptor() noexcept -> wgpu::RenderPassDescriptor {
	m_ColorAttachments.resize(1);
	m_ColorAttachments[0].view		 = view();
	m_ColorAttachments[0].loadOp	 = wgpu::LoadOp::Clear;
	m_ColorAttachments[0].storeOp	 = wgpu::StoreOp::Store;
	m_ColorAttachments[0].clearValue = {m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]};

	auto pass_descriptor				 = wgpu::RenderPassDescriptor();
	pass_descriptor.colorAttachments	 = m_ColorAttachments.data();
	pass_descriptor.colorAttachmentCount = m_ColorAttachments.size();


	return pass_descriptor;
}
