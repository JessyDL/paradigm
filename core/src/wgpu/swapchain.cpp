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
	: m_UseDepth(use_depth), m_Context(context) {
	m_SwapChain = m_Context->surface();

	wgpu::SurfaceCapabilities caps;
	m_SwapChain.GetCapabilities(m_Context->adapter(), &caps);

	wgpu::SurfaceConfiguration config = {};
	config.usage					  = wgpu::TextureUsage::RenderAttachment;
	config.format					  = caps.formats[0];	// todo make this configurable
	config.width					  = surface->data().width();
	config.height					  = surface->data().height();
	config.presentMode				  = wgpu::PresentMode::Fifo;	// todo make this configurable
	config.device					  = m_Context->device();

	m_SwapChain.Configure(&config);
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
