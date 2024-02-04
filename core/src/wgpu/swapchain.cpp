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
