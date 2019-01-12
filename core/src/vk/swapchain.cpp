#include "vk/swapchain.h"
#include "os/surface.h"
#include "vk/context.h"
#include "vk/texture.h"
#include "meta/texture.h"
#include "logging.h"

using namespace psl;
using namespace core;
using namespace core::gfx;
using namespace core::os;
using namespace core::resource;

swapchain::swapchain(const UID& uid, cache& cache, handle<core::os::surface> surface,
					 handle<core::gfx::context> context, bool use_depth)
	: m_OSSurface(surface), m_Context(context), m_Cache(cache), m_DepthTextureHandle(create<texture>(cache)),
	  m_UseDepth(use_depth)
{
#ifdef SURFACE_WIN32
	vk::Win32SurfaceCreateInfoKHR createInfo;
	createInfo.hinstance = m_OSSurface->surface_instance();
	createInfo.hwnd		 = m_OSSurface->surface_handle();

	utility::vulkan::check(m_Context->instance().createWin32SurfaceKHR(&createInfo, VK_NULL_HANDLE, &m_Surface));
#elif defined(SURFACE_XCB)
	vk::XcbSurfaceCreateInfoKHR createInfo;
	createInfo.connection = m_OSSurface->connection();
	createInfo.window	 = m_OSSurface->surface_handle();
	utility::vulkan::check(m_Context->instance().createXcbSurfaceKHR(&createInfo, VK_NULL_HANDLE, &m_Surface));
#elif defined(SURFACE_WAYLAND)
	vk::WaylandSurfaceCreateInfoKHR createInfo;
	createInfo.display = m_OSSurface->display();
	createInfo.surface = m_OSSurface->surface_handle();
	utility::vulkan::check(m_Context->instance().createWaylandSurfaceKHR(&createInfo, VK_NULL_HANDLE, &m_Surface));
#elif defined(PLATFORM_ANDROID)
	vk::AndroidSurfaceCreateInfoKHR createInfo;
	core::ivk::log->info("creating android swapchain");
	auto android_app = platform::specifics::android_application;
	auto window = android_app->window;
	createInfo.window = window;
	utility::vulkan::check(m_Context->instance().createAndroidSurfaceKHR(&createInfo, VK_NULL_HANDLE, &m_Surface));
#elif defined(SURFACE_D2D)
	uint32_t displayPropertyCount;
	std::vector<vk::DisplayPropertiesKHR> displayProperties;
	// Get display property
	m_Context->physical_device().getDisplayPropertiesKHR(&displayPropertyCount, nullptr);
	displayProperties.resize(displayPropertyCount);
	m_Context->physical_device().getDisplayPropertiesKHR(&displayPropertyCount, displayProperties.data());

	// Get plane property
	uint32_t planePropertyCount;
	std::vector<vk::DisplayPlanePropertiesKHR> planeProperties;
	m_Context->physical_device().getDisplayPlanePropertiesKHR(&planePropertyCount, nullptr);
	planeProperties.resize(planePropertyCount);
	m_Context->physical_device().getDisplayPlanePropertiesKHR(&planePropertyCount, planeProperties.data());

	vk::DisplayKHR display;
	vk::DisplayModeKHR displayMode;
	bool foundMode = false;
	for(uint32_t i = 0; i < displayPropertyCount; ++i)
	{
		display = displayProperties[i].display;
		uint32_t modeCount;
		std::vector<vk::DisplayModePropertiesKHR> modeProperties;
		m_Context->physical_device().getDisplayModePropertiesKHR(display, &modeCount, nullptr);
		modeProperties.resize(modeCount);
		m_Context->physical_device().getDisplayModePropertiesKHR(display, &modeCount, modeProperties.data());

		for(uint32_t j = 0; j < modeCount; ++j)
		{
			const auto* mode = &modeProperties[j];

			if(mode->parameters.visibleRegion.width == m_OSSurface->data().width() &&
			   mode->parameters.visibleRegion.height == m_OSSurface->data().height())
			{
				displayMode = mode->displayMode;
				foundMode   = true;
				break;
			}
		}
		if(foundMode)
		{
			break;
		}
	}

	if(!foundMode)
	{
		core::ivk::log->critical("couldn't find a display and display mode that matched");
		exit(-1);
	}

	// Search for a best plane we can use
	uint32_t bestPlaneIndex = UINT32_MAX;
	for(uint32_t i = 0; i < planePropertyCount; i++)
	{
		uint32_t planeIndex = i;
		uint32_t displayCount;
		std::vector<vk::DisplayKHR> displays;
		m_Context->physical_device().getDisplayPlaneSupportedDisplaysKHR(planeIndex, &displayCount, nullptr);
		displays.resize(displayCount);
		m_Context->physical_device().getDisplayPlaneSupportedDisplaysKHR(planeIndex, &displayCount, displays.data());

		// Find a display that matches the current plane
		bestPlaneIndex = UINT32_MAX;
		for(uint32_t j = 0; j < displayCount; j++)
		{
			if(display == displays[j])
			{
				bestPlaneIndex = i;
				break;
			}
		}
		if(bestPlaneIndex != UINT32_MAX)
		{
			break;
		}
	}

	if(bestPlaneIndex == UINT32_MAX)
	{
		core::ivk::log->critical("couldn't find a plane to display on");
		exit(-1);
	}

	vk::DisplayPlaneCapabilitiesKHR planeCap;
	m_Context->physical_device().getDisplayPlaneCapabilitiesKHR(displayMode, bestPlaneIndex, &planeCap);
	vk::DisplayPlaneAlphaFlagBitsKHR alphaMode;

	if(planeCap.supportedAlpha & vk::DisplayPlaneAlphaFlagBitsKHR::ePerPixelPremultiplied)
	{
		alphaMode = vk::DisplayPlaneAlphaFlagBitsKHR::ePerPixelPremultiplied;
	}
	else if(planeCap.supportedAlpha & vk::DisplayPlaneAlphaFlagBitsKHR::ePerPixel)
	{
		alphaMode = vk::DisplayPlaneAlphaFlagBitsKHR::ePerPixel;
	}
	else if(planeCap.supportedAlpha & vk::DisplayPlaneAlphaFlagBitsKHR::eGlobal)
	{
		alphaMode = vk::DisplayPlaneAlphaFlagBitsKHR::eGlobal;
	}
	else if(planeCap.supportedAlpha & vk::DisplayPlaneAlphaFlagBitsKHR::eOpaque)
	{
		alphaMode = vk::DisplayPlaneAlphaFlagBitsKHR::eOpaque;
	}

	vk::DisplaySurfaceCreateInfoKHR surfaceInfo;
	surfaceInfo.pNext			   = nullptr;
	surfaceInfo.displayMode		   = displayMode;
	surfaceInfo.planeIndex		   = bestPlaneIndex;
	surfaceInfo.planeStackIndex	= planeProperties[bestPlaneIndex].currentStackIndex;
	surfaceInfo.transform		   = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	surfaceInfo.globalAlpha		   = 1.0;
	surfaceInfo.alphaMode		   = alphaMode;
	surfaceInfo.imageExtent.width  = m_OSSurface->data().width();
	surfaceInfo.imageExtent.height = m_OSSurface->data().height();

	if(utility::vulkan::check(
		   m_Context->instance().createDisplayPlaneSurfaceKHR(&surfaceInfo, VK_NULL_HANDLE, &m_Surface)))
	{
		core::ivk::log->critical("failed to create the D2D surface");
		exit(-1);
	}
#else
#error platform not supported by the swapchain
#endif

	init_surface();
	init_swapchain();
	init_command_buffer();
	init_images();
	if(m_UseDepth) init_depthstencil();
	init_renderpass();
	init_framebuffer();
	flush();
}

swapchain::~swapchain()
{
	deinit_framebuffer();
	deinit_renderpass();
	if(m_UseDepth) deinit_depthstencil();
	deinit_images();
	deinit_command_buffer();
	deinit_swapchain();
	deinit_surface();
}

void swapchain::init_surface()
{
	m_ImageCount		= (uint32_t)(m_OSSurface->data().buffering());
	auto physicalDevice = m_Context->physical_device();
	utility::vulkan::check(
		physicalDevice.getSurfaceSupportKHR(m_Context->graphics_queue_index(), m_Surface, &m_SupportKHR));
	if(m_SupportKHR == VK_FALSE)
	{
		LOG_FATAL("Does not support KHR surface.");
	}
	utility::vulkan::check(physicalDevice.getSurfaceCapabilitiesKHR(m_Surface, &m_SurfaceCapabilities));
	if(m_SurfaceCapabilities.currentExtent.width < UINT32_MAX)
	{
		// TODO: Figure out this problematic area
		// width = m_SurfaceCapabilities.currentExtent.width;
		// height = m_SurfaceCapabilities.currentExtent.height;
		if(m_SurfaceCapabilities.maxImageCount != 0)
		{
			m_ImageCount = (uint32_t)std::fmin(m_ImageCount, m_SurfaceCapabilities.maxImageCount);
		}
		m_ImageCount = (uint32_t)std::fmax(m_ImageCount, m_SurfaceCapabilities.minImageCount);
	}

	if(auto res = physicalDevice.getSurfaceFormatsKHR(m_Surface); utility::vulkan::check(res.result))
	{
		m_SurfaceSupportFormats = res.value;
	}
	else
	{
		core::ivk::log->critical("could not get surface formats");
	}
	if(m_SurfaceSupportFormats.size() <= 0) LOG_FATAL("No surface formats found.");

	if(m_SurfaceSupportFormats[0].format == vk::Format::eUndefined)
	{
		m_SurfaceFormat.format	 = SURFACE_FORMAT;
		m_SurfaceFormat.colorSpace = SURFACE_COLORSPACE;
	}
	else
	{
		m_SurfaceFormat = m_SurfaceSupportFormats[0];
		for(auto& item : m_SurfaceSupportFormats)
		{
			if(item.format == SURFACE_FORMAT)
			{
				m_SurfaceFormat.format = SURFACE_FORMAT;
				break;
			}
		}
		m_SurfaceFormat.colorSpace = SURFACE_COLORSPACE;
	}
}
void swapchain::deinit_surface() { m_Context->instance().destroySurfaceKHR(m_Surface); }

void swapchain::init_swapchain(std::optional<vk::SwapchainKHR> previous)
{
	auto physicalDevice			   = m_Context->physical_device();
	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
	{
		std::vector<vk::PresentModeKHR> allPresentModes;
		if(auto res = physicalDevice.getSurfacePresentModesKHR(m_Surface); utility::vulkan::check(res.result))
			allPresentModes = res.value;

		presentMode = (std::find(allPresentModes.begin(), allPresentModes.end(), vk::PresentModeKHR::eMailbox) !=
					   allPresentModes.end())
						  ? vk::PresentModeKHR::eMailbox
						  : vk::PresentModeKHR::eFifo;
	}

	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.surface				 = m_Surface;
	createInfo.minImageCount		 = m_ImageCount;
	createInfo.imageFormat			 = m_SurfaceFormat.format;
	createInfo.imageColorSpace		 = m_SurfaceFormat.colorSpace;
	createInfo.imageExtent.width	 = (uint32_t)m_OSSurface->data().width();
	createInfo.imageExtent.height	= (uint32_t)m_OSSurface->data().height();
	createInfo.imageArrayLayers		 = 1;
	createInfo.imageUsage			 = vk::ImageUsageFlagBits::eColorAttachment; // we need to draw into it;
	createInfo.imageSharingMode		 = vk::SharingMode::eExclusive;
	createInfo.queueFamilyIndexCount = m_Context->graphics_queue_index(); // ignored if VK_SHARING_MODE_EXCLUSIVE
	createInfo.pQueueFamilyIndices   = nullptr;							  // ignored if VK_SHARING_MODE_EXCLUSIVE
	createInfo.preTransform			 = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	createInfo.compositeAlpha		 = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode			 = presentMode;
	createInfo.clipped = VK_TRUE; // Setting clipped to VK_TRUE allows the implementation to discard rendering outside
								  // of the surface area
	if(previous)
		createInfo.oldSwapchain = previous.value();
	else
		createInfo.oldSwapchain = nullptr;

	utility::vulkan::check(m_Context->device().createSwapchainKHR(&createInfo, nullptr, &m_Swapchain));

	utility::vulkan::check(m_Context->device().getSwapchainImagesKHR(m_Swapchain, &m_SwapchainImageCount, nullptr));
	if(m_SwapchainImageCount <= 0) LOG_FATAL("We received no swapchain images.");
}

void swapchain::deinit_swapchain() { m_Context->device().destroySwapchainKHR(m_Swapchain); }

void swapchain::init_images()
{
	m_SwapchainImages.resize(m_SwapchainImageCount);
	m_SwapchainImageViews.resize(m_SwapchainImageCount);

	auto device = m_Context->device();
	utility::vulkan::check(device.getSwapchainImagesKHR(m_Swapchain, &m_SwapchainImageCount, m_SwapchainImages.data()));

	for(uint32_t i = 0; i < m_SwapchainImageCount; ++i)
	{
		vk::ImageViewCreateInfo CI;
		CI.image	= m_SwapchainImages[i];
		CI.viewType = vk::ImageViewType::e2D;
		CI.format   = m_SurfaceFormat.format;

		CI.components.r					   = vk::ComponentSwizzle::eR;
		CI.components.g					   = vk::ComponentSwizzle::eG;
		CI.components.b					   = vk::ComponentSwizzle::eB;
		CI.components.a					   = vk::ComponentSwizzle::eA;
		CI.subresourceRange.aspectMask	 = vk::ImageAspectFlagBits::eColor;
		CI.subresourceRange.baseMipLevel   = 0;
		CI.subresourceRange.levelCount	 = 1;
		CI.subresourceRange.baseArrayLayer = 0;
		CI.subresourceRange.layerCount	 = 1;

		utility::vulkan::check(device.createImageView(&CI, nullptr, &m_SwapchainImageViews[i]));
	}
}

void swapchain::deinit_images()
{
	auto& device = m_Context->device();
	for(uint32_t i = 0; i < m_SwapchainImageCount; ++i) device.destroyImageView(m_SwapchainImageViews[i], nullptr);
}

void swapchain::init_command_buffer()
{
	vk::CommandBufferAllocateInfo AI;
	AI.commandBufferCount = 1;
	AI.commandPool		  = m_Context->command_pool();
	AI.level			  = vk::CommandBufferLevel::ePrimary;

	if(!utility::vulkan::check(m_Context->device().allocateCommandBuffers(&AI, &m_SetupCommandBuffer)))
	{
		LOG_ERROR("Could not allocate primary command buffer!");
	}
	vk::CommandBufferBeginInfo cmdBufInfo;

	utility::vulkan::check(m_SetupCommandBuffer.begin(&cmdBufInfo));
}

void swapchain::flush()
{
	if(!m_SetupCommandBuffer) return;

	utility::vulkan::check(m_SetupCommandBuffer.end());

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers	= &m_SetupCommandBuffer;

	auto queue = m_Context->queue();
	utility::vulkan::check(queue.submit(1, &submitInfo, nullptr));

	utility::vulkan::check(queue.waitIdle());

	m_Context->device().freeCommandBuffers(m_Context->command_pool(), 1, &m_SetupCommandBuffer);
	m_SetupCommandBuffer = nullptr;
}

void swapchain::deinit_command_buffer()
{
	if(!m_SetupCommandBuffer) return;
	m_Context->device().freeCommandBuffers(m_Context->command_pool(), 1, &m_SetupCommandBuffer);
}

void swapchain::init_depthstencil()
{
	// auto device = m_Context->device();
	vk::Format depthFormat;
	if(utility::vulkan::supported_depthformat(m_Context->physical_device(), &depthFormat) != VK_TRUE)
	{
		LOG_FATAL("Could not find a suitable depth stencil buffer format.");
	}

	auto [metaUID, metaData] = m_Cache.library().create<meta::texture>();
	m_Cache.library().set(metaUID, "SCDepthStencil");
	metaData.width(m_OSSurface->data().width());
	metaData.height(m_OSSurface->data().height());
	metaData.depth(1);
	metaData.mip_levels(1);
	metaData.image_type(vk::ImageViewType::e2D);
	metaData.format(depthFormat);
	metaData.usage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc);
	metaData.aspect_mask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);

	m_DepthTextureHandle = core::resource::create<core::gfx::texture>(m_Cache, metaUID);
	m_DepthTextureHandle.load(m_Context);
}

void swapchain::deinit_depthstencil() { m_DepthTextureHandle.unload(); }

void swapchain::init_renderpass()
{
	std::array<vk::AttachmentDescription, 2> attachments;

	// Color attachment
	attachments[0].format		  = m_SurfaceFormat.format;
	attachments[0].samples		  = vk::SampleCountFlagBits::e1;
	attachments[0].loadOp		  = vk::AttachmentLoadOp::eClear;
	attachments[0].storeOp		  = vk::AttachmentStoreOp::eStore;
	attachments[0].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
	attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[0].initialLayout  = vk::ImageLayout::eUndefined;
	attachments[0].finalLayout	= vk::ImageLayout::ePresentSrcKHR;

	// Depth attachment
	attachments[1].format		  = m_DepthTextureHandle->meta().format();
	attachments[1].samples		  = vk::SampleCountFlagBits::e1;
	attachments[1].loadOp		  = vk::AttachmentLoadOp::eClear;
	attachments[1].storeOp		  = vk::AttachmentStoreOp::eStore;
	attachments[1].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
	attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[1].initialLayout  = vk::ImageLayout::eUndefined;
	attachments[1].finalLayout	= vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference colorReference;
	colorReference.attachment = 0;
	colorReference.layout	 = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference depthReference;
	depthReference.attachment = 1;
	depthReference.layout	 = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint		= vk::PipelineBindPoint::eGraphics;
	subpass.inputAttachmentCount	= 0;
	subpass.pInputAttachments		= NULL;
	subpass.colorAttachmentCount	= 1;
	subpass.pColorAttachments		= &colorReference;
	subpass.pResolveAttachments		= NULL;
	subpass.pDepthStencilAttachment = &depthReference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments	= NULL;

	// Setup subpass dependencies
	// These will add the implicit ttachment layout transitionss specified by the attachment descriptions
	// The actual usage layout is preserved through the layout specified in the attachment reference
	// Each subpass dependency will introduce a memory and execution dependency between the source and dest subpass
	// described by srcStageMask, dstStageMask, srcAccessMask, dstAccessMask (and dependencyFlags is set) Note:
	// VK_SUBPASS_EXTERNAL is a special constant that refers to all commands executed outside of the actual renderpass)
	std::array<vk::SubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass	= VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass	= 0;
	dependencies[0].srcStageMask  = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[0].dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
	dependencies[0].dstAccessMask =
		vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	dependencies[1].srcSubpass   = 0;
	dependencies[1].dstSubpass   = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[1].srcAccessMask =
		vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
	dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.pNext		   = NULL;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments	= attachments.data();
	renderPassInfo.subpassCount	= 1;
	renderPassInfo.pSubpasses	  = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies   = dependencies.data();

	utility::vulkan::check(m_Context->device().createRenderPass(&renderPassInfo, nullptr, &m_RenderPass));
}

void swapchain::deinit_renderpass() { m_Context->device().destroyRenderPass(m_RenderPass); }

void swapchain::init_framebuffer()
{
	vk::ImageView attachments[2];

	// Depth/Stencil attachment is the same for all frame buffers
	attachments[1] = m_DepthTextureHandle->view();

	vk::FramebufferCreateInfo frameBufferCreateInfo;
	frameBufferCreateInfo.pNext			  = NULL;
	frameBufferCreateInfo.renderPass	  = m_RenderPass;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments	= attachments;
	frameBufferCreateInfo.width			  = m_OSSurface->data().width();
	frameBufferCreateInfo.height		  = m_OSSurface->data().height();
	frameBufferCreateInfo.layers		  = 1;

	// Create frame buffers for every swap chain image
	m_Framebuffer.resize(m_SwapchainImageCount);
	for(uint32_t i = 0; i < m_SwapchainImageCount; i++)
	{
		attachments[0] = m_SwapchainImageViews[i];
		utility::vulkan::check(
			m_Context->device().createFramebuffer(&frameBufferCreateInfo, nullptr, &m_Framebuffer[i]));
	}
}

void swapchain::deinit_framebuffer()
{
	for(uint32_t i = 0; i < m_SwapchainImageCount; ++i)
		m_Context->device().destroyFramebuffer(m_Framebuffer[i], nullptr);
}

bool swapchain::next(vk::Semaphore presentComplete, uint32_t& out_image_index)
{
	bool recreated = false;
	if(m_ShouldResize)
	{
		recreated = true;
		apply_resize();
	}
	// vk::AcquireNextImageInfoKHR acquireNextImageInfo;
	// acquireNextImageInfo.swapchain = m_Swapchain;
	// acquireNextImageInfo.timeout   = UINT64_MAX;
	// acquireNextImageInfo.semaphore = presentComplete;
	// acquireNextImageInfo.fence	 = nullptr;

	// auto resultValue  = m_Context->device().acquireNextImage2KHR(acquireNextImageInfo);
	auto resultValue  = m_Context->device().acquireNextImageKHR(m_Swapchain, UINT64_MAX, presentComplete, nullptr);
	vk::Result result = resultValue.result;
	if((result == vk::Result::eErrorOutOfDateKHR) || (result == vk::Result::eSuboptimalKHR))
	{
		recreated = true;
		apply_resize();
	}
	else if(result != vk::Result::eSuccess)
	{
		std::cerr << "Invalid acquire result: " << vk::to_string(result);
	}

	m_CurrentImage  = resultValue.value;
	out_image_index = m_CurrentImage;
	return recreated;
}
vk::Result swapchain::present(vk::Semaphore wait)
{
	vk::PresentInfoKHR presentInfo;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains	= &m_Swapchain;
	presentInfo.pImageIndices  = &m_CurrentImage;

	presentInfo.waitSemaphoreCount = wait ? 1 : 0;
	presentInfo.pWaitSemaphores	= &wait;
	auto res					   = m_Context->queue().presentKHR(presentInfo);
	if(!((res == vk::Result::eSuccess) || (res == vk::Result::eSuboptimalKHR)))
	{
		if(res == vk::Result::eErrorOutOfDateKHR)
		{
			apply_resize();
		}
	}
	return res;
}

void swapchain::resize()
{
	while(m_Resizing)
	{
	};

	m_ShouldResize = true;
}
void swapchain::apply_resize()
{
	m_Resizing = true;
	// hard sync.
	utility::vulkan::check(m_Context->device().waitIdle());

	// recreate the swapchain
	vk::SwapchainKHR previous = m_Swapchain;
	auto images				  = m_SwapchainImageCount;
	init_surface();
	init_swapchain(previous);


	deinit_framebuffer();
	deinit_renderpass();
	auto& device = m_Context->device();
	for(uint32_t i = 0; i < images; ++i) device.destroyImageView(m_SwapchainImageViews[i], nullptr);
	if(m_UseDepth) deinit_depthstencil();
	deinit_command_buffer();
	device.destroySwapchainKHR(previous);

	init_command_buffer();
	init_images();
	if(m_UseDepth) init_depthstencil();
	init_renderpass();
	init_framebuffer();
	// hard sync.
	utility::vulkan::check(m_Context->device().waitIdle());
	m_Resizing	 = false;
	m_ShouldResize = false;
}

bool swapchain::is_ready() const noexcept { return !m_Resizing; }

// amount of images in the swapchain
uint32_t swapchain::size() const noexcept { return m_SwapchainImageCount; }
vk::RenderPass swapchain::renderpass() const noexcept { return m_RenderPass; }

uint32_t swapchain::width() const noexcept { return m_OSSurface->data().width(); }
uint32_t swapchain::height() const noexcept { return m_OSSurface->data().height(); }

const std::vector<vk::Framebuffer>& swapchain::framebuffers() const noexcept { return m_Framebuffer; }
const std::vector<vk::Image>& swapchain::images() const noexcept { return m_SwapchainImages; }
const std::vector<vk::ImageView>& swapchain::views() const noexcept { return m_SwapchainImageViews; }
const vk::ClearColorValue swapchain::clear_color() const noexcept { return m_ClearColor; }
const vk::ClearDepthStencilValue swapchain::clear_depth() const noexcept { return m_ClearDepth; }
bool swapchain::has_depth() const noexcept { return m_UseDepth; }

void swapchain::clear_color(vk::ClearColorValue color) noexcept
{
	m_ClearColor = color;
}