#pragma once

#include "core/fwd/wgpu/texture.hpp"
#include "core/resource/handle.hpp"
#include "core/wgpu/iwgpu.hpp"

namespace core::os {
class surface;
}

namespace core::iwgpu {
class context;
class swapchain {
  public:
	swapchain(core::resource::cache_t& cache,
			  const core::resource::metadata& metaData,
			  psl::meta::file* metaFile,
			  core::resource::handle<core::os::surface> surface,
			  core::resource::handle<core::iwgpu::context> context,
			  bool use_depth = true);
	~swapchain() = default;

	swapchain(const swapchain& other)				 = default;
	swapchain(swapchain&& other) noexcept			 = default;
	swapchain& operator=(const swapchain& other)	 = default;
	swapchain& operator=(swapchain&& other) noexcept = default;
	bool present();

	auto views() const noexcept -> std::span<wgpu::RenderPassColorAttachment const> {
		return {std::begin(m_ColorAttachments), std::end(m_ColorAttachments)};
	}

	auto texture() const noexcept -> wgpu::Texture { return m_SwapChain.GetCurrentTexture(); }
	auto view() const noexcept -> wgpu::TextureView { return m_SwapChain.GetCurrentTextureView(); }

  private:
	psl::vec4 m_ClearColor {0.25f, 0.4f, 0.95f, 1.0f};
	float m_ClearDepth {1.0f};
	uint32_t m_ClearStencil {0};
	bool m_UseDepth {false};

	wgpu::SwapChain m_SwapChain {};
	std::vector<wgpu::RenderPassColorAttachment> m_ColorAttachments {};
	std::vector<wgpu::RenderPassDepthStencilAttachment> m_DepthStencilAttachments {};
};
}	 // namespace core::iwgpu
