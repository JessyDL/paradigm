#pragma once
#include "resource/handle.h"
#include "vk/stdafx.h"
#include "fwd/vk/texture.h"

namespace core::os
{
	class surface;
}

namespace core::ivk
{
	class texture;
	class context;
	class framebuffer;
}

namespace core::data
{
	class framebuffer;
}

namespace core::ivk
{
	/// \brief describes a framebuffer that is specially handled and created by the driver
	///
	/// swpachains can be considered special core::ivk::framebuffer's, with some special
	/// pecularities. For example, unlike framebuffer's, you have to request the next image
	/// from the driver for a swapchain, and there are many more restrictions on both format
	/// and size imposed on swapchains.
	class swapchain
	{
		friend class core::os::surface;

	  public:
		swapchain(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				  core::resource::handle<core::os::surface> surface,
				  core::resource::handle<core::ivk::context> context, bool use_depth = true);
		~swapchain();

		/// \returns true in case it managed to get the next image in the swapchain from the driver.
		/// \param[in, out] presentComplete the semaphore to flag once ready.
		/// \param[out] out_image_index the image index of the next image in the swapchain.
		/// \details when invoking this method it will request the next image in the swapchain, and return.
		/// the semaphore will be flaged once the resource is ready on the GPU. (and so should be used as a sync point
		/// before using the image in the swapchain.
		bool next(vk::Semaphore presentComplete, uint32_t& out_image_index);

		/// \returns success in case presenting went well.
		/// \param[in] wait the semaphore to wait on when presenting.
		vk::Result present(vk::Semaphore wait);

		/// returns the amount of images in the swapchain
		uint32_t size() const noexcept;
		/// \returns the renderpass associated with the swapchain.
		vk::RenderPass renderpass() const noexcept;

		/// \returns the width of the swapchain image
		uint32_t width() const noexcept;
		/// \returns the height of the swapchainimage.
		uint32_t height() const noexcept;

		/// \returns the vulkan framebuffers for the swapchain (one for each buffer).
		const std::vector<vk::Framebuffer>& framebuffers() const noexcept;
		/// \returns the images for the swapchain (one for each buffer).
		const std::vector<vk::Image>& images() const noexcept;
		/// \returns the image views for the swapchain (one for each buffer).
		const std::vector<vk::ImageView>& views() const noexcept;
		/// \returns the clear color value assigned to the swapchain.
		const vk::ClearColorValue clear_color() const noexcept;
		/// \returns the clear depth color of the swapchain
		/// \warning will return default value in case there is no depth texture.
		const vk::ClearDepthStencilValue clear_depth() const noexcept;

		/// \returns if the swapchain uses a depth texture or not.
		bool has_depth() const noexcept;
		/// \brief sets the clear color
		/// \param[in] color the new clear color value to use
		void clear_color(vk::ClearColorValue color) noexcept;

		/// \returns false in case the window might be resizing.
		/// \warning calls to getting the next swapchain image and presenting will fail in that scenario.
		bool is_ready() const noexcept;

	  private:
		void init_surface();
		void deinit_surface();

		void init_swapchain(std::optional<vk::SwapchainKHR> previous = std::nullopt);
		void deinit_swapchain();

		void init_images();
		void deinit_images();

		void init_command_buffer();
		void deinit_command_buffer();

		void init_depthstencil();
		void deinit_depthstencil();

		void init_renderpass();
		void deinit_renderpass();

		void init_framebuffer();
		void deinit_framebuffer();

		void flush();

		// this is called in the case of a resize event.
		void resize();
		void apply_resize();

		core::os::surface* m_OSSurface;
		core::resource::handle<core::ivk::context> m_Context;

		vk::SurfaceKHR m_Surface;
		vk::SurfaceCapabilitiesKHR m_SurfaceCapabilities;
		vk::SurfaceFormatKHR m_SurfaceFormat;
		std::vector<vk::SurfaceFormatKHR> m_SurfaceSupportFormats;
		vk::Bool32 m_SupportKHR;

		vk::SwapchainKHR m_Swapchain;
		uint32_t m_SwapchainImageCount = 0;
		std::vector<vk::Image> m_SwapchainImages;
		std::vector<vk::ImageView> m_SwapchainImageViews;
		uint32_t m_ImageCount = 0;

		vk::CommandBuffer m_SetupCommandBuffer;

		vk::RenderPass m_RenderPass;

		std::vector<vk::Framebuffer> m_Framebuffer;

		uint32_t m_CurrentImage = 0;

		core::resource::cache& m_Cache;
		core::resource::handle<core::ivk::texture> m_DepthTextureHandle;

		const bool m_UseDepth;
		vk::ClearColorValue m_ClearColor{std::array<float, 4>{0.25f, 0.4f, 0.95f, 1.0f}};
		vk::ClearDepthStencilValue m_ClearDepth{vk::ClearDepthStencilValue(1.0f, 0U)};

		bool m_Resizing;
		bool m_ShouldResize{false};
	};
} // namespace core::gfx
