#include "stdafx.h"
#include "gfx/pass.h"
#include "vk/context.h"
#include "vk/framebuffer.h"
#include "data/framebuffer.h"
#include "vk/swapchain.h"
#include "gfx/drawgroup.h"

using namespace core::resource;
using namespace core::gfx;


pass::pass(handle<context> context, handle<framebuffer> framebuffer)
	: m_Context(context), m_Framebuffer(framebuffer), m_UsingSwap(false)
{
	vk::SemaphoreCreateInfo semaphoreCreateInfo;
	semaphoreCreateInfo.pNext = NULL;

	utility::vulkan::check(m_Context->device().createSemaphore(&semaphoreCreateInfo, nullptr, &m_PresentComplete));
	utility::vulkan::check(m_Context->device().createSemaphore(&semaphoreCreateInfo, nullptr, &m_RenderComplete));

	m_DrawCommandBuffers.resize(m_Framebuffer->framebuffers().size());

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime

	m_SubmitInfo.pWaitDstStageMask	= &m_SubmitPipelineStages;
	m_SubmitInfo.waitSemaphoreCount   = 1;
	m_SubmitInfo.pWaitSemaphores	  = &m_PresentComplete;
	m_SubmitInfo.signalSemaphoreCount = 1;
	m_SubmitInfo.pSignalSemaphores	= &m_RenderComplete;

	create_fences(m_Framebuffer->framebuffers().size());

	build();
}

pass::pass(handle<context> context, handle<swapchain> swapchain)
	: m_Context(context), m_Swapchain(swapchain), m_UsingSwap(true)
{
	vk::SemaphoreCreateInfo semaphoreCreateInfo;
	semaphoreCreateInfo.pNext = NULL;

	utility::vulkan::check(m_Context->device().createSemaphore(&semaphoreCreateInfo, nullptr, &m_PresentComplete));
	utility::vulkan::check(m_Context->device().createSemaphore(&semaphoreCreateInfo, nullptr, &m_RenderComplete));

	m_DrawCommandBuffers.resize(m_Swapchain->framebuffers().size());

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime

	m_SubmitInfo.pWaitDstStageMask	= &m_SubmitPipelineStages;
	m_SubmitInfo.waitSemaphoreCount   = 1;
	m_SubmitInfo.pWaitSemaphores	  = &m_PresentComplete;
	m_SubmitInfo.signalSemaphoreCount = 1;
	m_SubmitInfo.pSignalSemaphores	= &m_RenderComplete;

	create_fences(m_Swapchain->framebuffers().size());

	//build();
}

pass::~pass()
{
	destroy_fences();
	m_Context->device().destroySemaphore(m_PresentComplete);
	m_Context->device().destroySemaphore(m_RenderComplete);
}
bool pass::build()
{
	LOG_INFO("Rebuilding Command Buffers");
	m_LastBuildFrame = m_FrameCount;

	m_Context->device().freeCommandBuffers(m_Context->command_pool(), m_DrawCommandBuffers.size(),
										   m_DrawCommandBuffers.data());

	m_Buffers = m_DrawCommandBuffers.size();

	vk::CommandBufferAllocateInfo cmdBufAllocateInfo;
	cmdBufAllocateInfo.commandPool		  = m_Context->command_pool();
	cmdBufAllocateInfo.commandBufferCount = (uint32_t)m_DrawCommandBuffers.size(); // one for each image
	cmdBufAllocateInfo.level			  = vk::CommandBufferLevel::ePrimary;

	if(!utility::vulkan::check(
		   m_Context->device().allocateCommandBuffers(&cmdBufAllocateInfo, m_DrawCommandBuffers.data())))
		throw new std::runtime_error("Critical issue");

	vk::CommandBufferBeginInfo cmdBufInfo;
	cmdBufInfo.pNext = NULL;

	std::vector<vk::ClearValue> clearValues;

	if(m_UsingSwap)
	{
		clearValues.reserve(1 + (size_t)m_Swapchain->has_depth());
		for(auto i = 0; i < 1; ++i)
		{
			clearValues.emplace_back(m_Swapchain->clear_color());
		}
		if(m_Swapchain->has_depth())
		{
			clearValues.emplace_back(m_Swapchain->clear_depth());
		}
	}
	else
	{
		const auto& attachments = m_Framebuffer->data()->attachments();
		clearValues.reserve(attachments.size());
		for(const auto& attach : attachments) clearValues.emplace_back(attach.clear_value());
	}
	bool success = false;
	for(auto i = 0; i < m_DrawCommandBuffers.size(); ++i)
	{

		m_Context->device().waitIdle();
		if(!utility::vulkan::check(m_DrawCommandBuffers[i].begin(cmdBufInfo)))
			throw new std::runtime_error("Critical issue");

		const std::vector<vk::Framebuffer>& framebuffers =
			(m_UsingSwap) ? m_Swapchain->framebuffers() : m_Framebuffer->framebuffers();


		vk::RenderPassBeginInfo renderPassBeginInfo;

		if(m_UsingSwap)
		{
			renderPassBeginInfo.renderPass				 = m_Swapchain->renderpass();
			renderPassBeginInfo.renderArea.extent.width  = m_Swapchain->width();
			renderPassBeginInfo.renderArea.extent.height = m_Swapchain->height();
		}
		else
		{
			renderPassBeginInfo.renderPass				 = m_Framebuffer->render_pass();
			renderPassBeginInfo.renderArea.extent.width  = m_Framebuffer->data()->width();
			renderPassBeginInfo.renderArea.extent.height = m_Framebuffer->data()->height();
		}

		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.clearValueCount		= (uint32_t)clearValues.size();
		renderPassBeginInfo.pClearValues		= clearValues.data();
		if(m_UsingSwap)
		{
			renderPassBeginInfo.framebuffer = framebuffers[i];

			m_DrawCommandBuffers[i].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

			// Update dynamic viewport state
			vk::Viewport viewport;
			viewport.height = (float)renderPassBeginInfo.renderArea.extent.height;
			viewport.width = (float)renderPassBeginInfo.renderArea.extent.width;
			viewport.minDepth = (float)0.0f;
			viewport.maxDepth = (float)1.0f;
			m_DrawCommandBuffers[i].setViewport(0, 1, &viewport);

			// Update dynamic scissor state
			vk::Rect2D scissor;
			scissor.extent.width = renderPassBeginInfo.renderArea.extent.width;
			scissor.extent.height = renderPassBeginInfo.renderArea.extent.height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			m_DrawCommandBuffers[i].setScissor(0, 1, &scissor);

			m_DrawCommandBuffers[i].setDepthBias(m_DepthBias.components[0], m_DepthBias.components[1],
													m_DepthBias.components[2]);

			for(auto& group : m_AllGroups) group.build(m_DrawCommandBuffers[i], m_Swapchain, i);

			m_DrawCommandBuffers[i].endRenderPass();			
		}
		else
		{
			for(const auto& framebuffer : framebuffers)
			{
				renderPassBeginInfo.framebuffer = framebuffer;

				m_DrawCommandBuffers[i].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

				// Update dynamic viewport state
				vk::Viewport viewport;
				viewport.height   = (float)renderPassBeginInfo.renderArea.extent.height;
				viewport.width	= (float)renderPassBeginInfo.renderArea.extent.width;
				viewport.minDepth = (float)0.0f;
				viewport.maxDepth = (float)1.0f;
				m_DrawCommandBuffers[i].setViewport(0, 1, &viewport);

				// Update dynamic scissor state
				vk::Rect2D scissor;
				scissor.extent.width  = renderPassBeginInfo.renderArea.extent.width;
				scissor.extent.height = renderPassBeginInfo.renderArea.extent.height;
				scissor.offset.x	  = 0;
				scissor.offset.y	  = 0;
				m_DrawCommandBuffers[i].setScissor(0, 1, &scissor);

				m_DrawCommandBuffers[i].setDepthBias(m_DepthBias.components[0], m_DepthBias.components[1],
													 m_DepthBias.components[2]);

			
				for(auto& group : m_AllGroups) group.build(m_DrawCommandBuffers[i], m_Framebuffer, i);
						

				m_DrawCommandBuffers[i].endRenderPass();
			}
		}

		success |= utility::vulkan::check(m_DrawCommandBuffers[i].end());
	}

	return success;
}

void pass::create_fences(const size_t size)
{
	if(m_WaitFences.size() > 0) destroy_fences();

	for(auto i = 0; i < size; ++i)
	{
		vk::FenceCreateInfo fCI;
		fCI.flags = vk::FenceCreateFlagBits::eSignaled;
		m_WaitFences.push_back(m_Context->device().createFence(fCI, nullptr).value);
	}
}

void pass::destroy_fences()
{
	for(auto& fence : m_WaitFences)
	{
		if(!utility::vulkan::check(m_Context->device().waitForFences(1, &fence, VK_TRUE, UINT_MAX)))
			LOG_ERROR("Failed to wait for fence");

		if(!utility::vulkan::check(m_Context->device().resetFences(1, &fence))) LOG_ERROR("Failed to reset fence");

		m_Context->device().destroyFence(fence, nullptr);
	}
	m_WaitFences.clear();
}

void pass::prepare()
{
	if(m_UsingSwap)
	{
		if(m_Swapchain->next(m_PresentComplete, m_CurrentBuffer)) build();
	}
	else
	{
		m_CurrentBuffer = 0u;
		// todo verify if this behaviour is correct, should we build here too?
	}
}

void pass::present()
{
	if(m_WaitFences.size() > 0)
	{
		if(!utility::vulkan::check(
			   m_Context->device().waitForFences(1, &m_WaitFences[m_CurrentBuffer], VK_TRUE, UINT_MAX)))
			LOG_ERROR("Failed to wait for fence");

		if(!utility::vulkan::check(m_Context->device().resetFences(1, &m_WaitFences[m_CurrentBuffer])))
			LOG_ERROR("Failed to reset fence");
	}

	std::vector<vk::Semaphore> semaphores;
	std::vector<vk::PipelineStageFlags> stageFlags;

	if(m_UsingSwap)
	{
		semaphores.push_back(m_PresentComplete);
	}

	if(semaphores.size() == 0)
	{
		m_SubmitInfo.pWaitSemaphores	= VK_NULL_HANDLE;
		m_SubmitInfo.waitSemaphoreCount = 0;
	}
	else
	{
		m_SubmitInfo.waitSemaphoreCount = semaphores.size();
		m_SubmitInfo.pWaitSemaphores	= semaphores.data();

		for(auto i = 0; i < semaphores.size(); ++i) stageFlags.push_back(m_SubmitPipelineStages);

		m_SubmitInfo.pWaitDstStageMask = stageFlags.data();
	}

	// Signal ready with offscreen semaphore
	m_SubmitInfo.pSignalSemaphores = &m_RenderComplete;

	m_SubmitInfo.pCommandBuffers	= &m_DrawCommandBuffers[m_CurrentBuffer];
	m_SubmitInfo.commandBufferCount = 1;


	if(m_WaitFences.size() > 0)
		utility::vulkan::check(m_Context->queue().submit(1, &m_SubmitInfo, m_WaitFences[m_CurrentBuffer]));
	else
		utility::vulkan::check(m_Context->queue().submit(1, &m_SubmitInfo, nullptr));

	if(m_UsingSwap)
	{
		utility::vulkan::check(m_Swapchain->present(m_RenderComplete));
	}

	++m_FrameCount;
}


void pass::bias(const core::gfx::depth_bias& bias) noexcept { m_DepthBias = bias; }
core::gfx::depth_bias pass::bias() const noexcept { return m_DepthBias; }

void pass::add(core::gfx::drawgroup& group) noexcept { m_AllGroups.push_back(group); }
void pass::remove(const core::gfx::drawgroup& group) noexcept
{
	m_AllGroups.erase(
		std::remove_if(std::begin(m_AllGroups), std::end(m_AllGroups),
					   [&group](const std::reference_wrapper<drawgroup>& element) { return &group == &element.get(); }),
		std::end(m_AllGroups));
}

void pass::clear() noexcept { m_AllGroups.clear(); }