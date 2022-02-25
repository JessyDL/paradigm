#pragma once
#include "psl/platform_def.hpp"
#include "psl/string_utils.hpp"

#define VULKAN_HPP_NO_EXCEPTIONS

#if defined(SURFACE_ANDROID)
	#if !defined(VK_USE_PLATFORM_ANDROID_KHR)
		#define VK_USE_PLATFORM_ANDROID_KHR
	#endif
	#define VK_SURFACE_EXTENSION_NAME VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#elif defined(SURFACE_XCB)
	#ifndef VK_USE_PLATFORM_XCB_KHR
	#define VK_USE_PLATFORM_XCB_KHR
	#endif
	#define VK_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif defined(SURFACE_D2D)
	#define VK_SURFACE_EXTENSION_NAME VK_KHR_DISPLAY_EXTENSION_NAME
#elif defined(SURFACE_WIN32)
	#define VK_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif

#define VK_VERSION_LATEST_MAJOR 1
#define VK_VERSION_LATEST_MINOR 2
#define VULKAN_HPP_NO_SMART_HANDLE

#include <vulkan/vulkan.hpp>

#define VK_VERSION_LATEST_PATCH VK_HEADER_VERSION
#define VK_API_VERSION_LATEST VK_API_VERSION_1_2

#include "psl/logging.hpp"
#include "psl/ustring.hpp"
#include "psl/assertions.hpp"
#include <unordered_map>

// Set to "true" to use staging buffers for uploading
// vertex and index data to device local memory
// See "prepareVertices" for details on what's staging
// and on why to use it
#define GPU_USE_STAGING true

#ifndef VK_FLAGS_NONE
#define VK_FLAGS_NONE 0
#endif

#define GAMMA_RENDERING
#ifndef GAMMA_RENDERING
#define SURFACE_FORMAT vk::Format::eB8G8R8A8Srgb
#define SURFACE_COLORSPACE vk::ColorSpaceKHR::eSrgbNonlinear
#else
#define SURFACE_FORMAT vk::Format::eB8G8R8A8Unorm
#define SURFACE_COLORSPACE vk::ColorSpaceKHR::eSrgbNonlinear
#endif

#ifdef DEBUG
#define VULKAN_ENABLE_VALIDATION true
#else
#define VULKAN_ENABLE_VALIDATION true
#endif


namespace utility
{
	template <typename BitType>
	struct converter<vk::Flags<BitType>>
	{
		static psl::string8_t to_string(const vk::Flags<BitType>& x)
		{
			using MaskType = typename vk::Flags<BitType>::MaskType;
			return converter<MaskType>::to_string((MaskType)(x));
		}

		static vk::Flags<BitType> from_string(psl::string8::view str)
		{
			using MaskType = typename vk::Flags<BitType>::MaskType;
			return (BitType)converter<MaskType>::from_string(str);
		}
	};

	template <>
	struct converter<vk::ClearValue>
	{
		static psl::string8_t to_string(const vk::ClearValue& value) { return ""; }

		static vk::ClearValue from_string(psl::string8::view str) { return vk::ClearValue {vk::ClearColorValue {}}; }
	};
}	 // namespace utility


/// \brief helper namespace that contains handy defaults and constructor helpers for Vulkan objects
namespace utility::vulkan::defaults
{
	/// \brief creates a default, 0 sized VkMemoryAllocateInfo
	inline VkMemoryAllocateInfo mem_ai()
	{
		VkMemoryAllocateInfo memAllocInfo = {};
		memAllocInfo.sType				  = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAllocInfo.pNext				  = NULL;
		memAllocInfo.allocationSize		  = 0;
		memAllocInfo.memoryTypeIndex	  = 0;
		return memAllocInfo;
	}

	inline VkCommandBufferAllocateInfo
	cmd_buffer_ai(VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t bufferCount)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType						  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool				  = commandPool;
		commandBufferAllocateInfo.level						  = level;
		commandBufferAllocateInfo.commandBufferCount		  = bufferCount;
		return commandBufferAllocateInfo;
	}

	inline VkCommandPoolCreateInfo cmd_pool_ci()
	{
		VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
		cmdPoolCreateInfo.sType					  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		return cmdPoolCreateInfo;
	}

	inline VkCommandBufferBeginInfo cmd_pool_bi()
	{
		VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
		cmdBufferBeginInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.pNext					= NULL;
		return cmdBufferBeginInfo;
	}

	inline VkCommandBufferInheritanceInfo cmd_buffer_ii()
	{
		VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo = {};
		cmdBufferInheritanceInfo.sType							= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		return cmdBufferInheritanceInfo;
	}

	inline VkRenderPassBeginInfo renderpass_bi()
	{
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType				  = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext				  = NULL;
		return renderPassBeginInfo;
	}

	inline VkRenderPassCreateInfo renderpass_ci()
	{
		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType					= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.pNext					= NULL;
		return renderPassCreateInfo;
	}

	inline VkImageMemoryBarrier image_membarrier()
	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext				= NULL;
		// Some default values
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		return imageMemoryBarrier;
	}

	inline VkBufferMemoryBarrier buffer_membarrier()
	{
		VkBufferMemoryBarrier bufferMemoryBarrier = {};
		bufferMemoryBarrier.sType				  = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferMemoryBarrier.pNext				  = NULL;
		return bufferMemoryBarrier;
	}

	inline VkSemaphoreCreateInfo semaphore_ci()
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType				  = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext				  = NULL;
		semaphoreCreateInfo.flags				  = 0;
		return semaphoreCreateInfo;
	}

	inline VkFenceCreateInfo fence_ci(VkFenceCreateFlags flags)
	{
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType			  = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags			  = flags;
		return fenceCreateInfo;
	}

	inline VkEventCreateInfo event_ci()
	{
		VkEventCreateInfo eventCreateInfo = {};
		eventCreateInfo.sType			  = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
		return eventCreateInfo;
	}

	inline VkSubmitInfo submit_i()
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType		= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext		= NULL;
		return submitInfo;
	}

	inline VkViewport viewport(float width, float height, float minDepth, float maxDepth)
	{
		VkViewport viewport = {};
		viewport.width		= width;
		viewport.height		= height;
		viewport.minDepth	= minDepth;
		viewport.maxDepth	= maxDepth;
		return viewport;
	}

	inline VkRect2D rect2D(int32_t width, int32_t height, int32_t offsetX, int32_t offsetY)
	{
		VkRect2D rect2D		 = {};
		rect2D.extent.width	 = width;
		rect2D.extent.height = height;
		rect2D.offset.x		 = offsetX;
		rect2D.offset.y		 = offsetY;
		return rect2D;
	}

	inline VkBufferCreateInfo buffer_ci()
	{
		VkBufferCreateInfo bufCreateInfo = {};
		bufCreateInfo.sType				 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		return bufCreateInfo;
	}

	inline VkBufferCreateInfo buffer_ci(VkBufferUsageFlags usage, VkDeviceSize size)
	{
		VkBufferCreateInfo bufCreateInfo = {};
		bufCreateInfo.sType				 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufCreateInfo.pNext				 = NULL;
		bufCreateInfo.usage				 = usage;
		bufCreateInfo.size				 = size;
		bufCreateInfo.flags				 = 0;
		return bufCreateInfo;
	}

	inline vk::DescriptorPoolCreateInfo
	descriptor_pool_ci(uint32_t poolSizeCount, vk::DescriptorPoolSize* pPoolSizes, uint32_t maxSets)
	{
		vk::DescriptorPoolCreateInfo descriptorPoolInfo;
		descriptorPoolInfo.pNext		 = NULL;
		descriptorPoolInfo.poolSizeCount = poolSizeCount;
		descriptorPoolInfo.pPoolSizes	 = pPoolSizes;
		descriptorPoolInfo.maxSets		 = maxSets;
		return descriptorPoolInfo;
	}

	inline VkDescriptorPoolSize descriptor_pool_size(VkDescriptorType type, uint32_t descriptorCount)
	{
		VkDescriptorPoolSize descriptorPoolSize = {};
		descriptorPoolSize.type					= type;
		descriptorPoolSize.descriptorCount		= descriptorCount;
		return descriptorPoolSize;
	}

	inline vk::DescriptorSetLayoutBinding descriptor_setlayout_binding(vk::DescriptorType type,
																	   vk::ShaderStageFlags stageFlags,
																	   uint32_t binding,
																	   uint32_t descriptorCount = 1)
	{
		vk::DescriptorSetLayoutBinding setLayoutBinding;
		setLayoutBinding.descriptorType = type;
		setLayoutBinding.stageFlags		= stageFlags;
		setLayoutBinding.binding		= binding;
		// Default value in all examples
		setLayoutBinding.descriptorCount	= descriptorCount;
		setLayoutBinding.pImmutableSamplers = nullptr;
		return setLayoutBinding;
	}

	inline VkDescriptorSetLayoutCreateInfo descriptor_setlayout_ci(const VkDescriptorSetLayoutBinding* pBindings,
																   uint32_t bindingCount)
	{
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
		descriptorSetLayoutCreateInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext		   = NULL;
		descriptorSetLayoutCreateInfo.pBindings	   = pBindings;
		descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
		return descriptorSetLayoutCreateInfo;
	}

	inline VkPipelineLayoutCreateInfo pipeline_layout_ci(const VkDescriptorSetLayout* pSetLayouts,
														 uint32_t setLayoutCount)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext						= NULL;
		pipelineLayoutCreateInfo.setLayoutCount				= setLayoutCount;
		pipelineLayoutCreateInfo.pSetLayouts				= pSetLayouts;
		return pipelineLayoutCreateInfo;
	}

	inline VkDescriptorSetAllocateInfo descriptor_set_ai(VkDescriptorPool descriptorPool,
														 const VkDescriptorSetLayout* pSetLayouts,
														 uint32_t descriptorSetCount)
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType						  = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pNext						  = NULL;
		descriptorSetAllocateInfo.descriptorPool			  = descriptorPool;
		descriptorSetAllocateInfo.pSetLayouts				  = pSetLayouts;
		descriptorSetAllocateInfo.descriptorSetCount		  = descriptorSetCount;
		return descriptorSetAllocateInfo;
	}

	inline vk::DescriptorImageInfo
	descriptor_image_info(vk::Sampler sampler, vk::ImageView imageView, vk::ImageLayout imageLayout)
	{
		vk::DescriptorImageInfo descriptorImageInfo;
		descriptorImageInfo.sampler		= sampler;
		descriptorImageInfo.imageView	= imageView;
		descriptorImageInfo.imageLayout = imageLayout;
		return descriptorImageInfo;
	}

	inline vk::WriteDescriptorSet write_descriptor_set(vk::DescriptorSet dstSet,
													   vk::DescriptorType type,
													   uint32_t binding,
													   vk::DescriptorBufferInfo* bufferInfo,
													   uint32_t descriptorCount = 1)
	{
		vk::WriteDescriptorSet writeDescriptorSet;
		writeDescriptorSet.pNext		  = NULL;
		writeDescriptorSet.dstSet		  = dstSet;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.dstBinding	  = binding;
		writeDescriptorSet.pBufferInfo	  = bufferInfo;
		writeDescriptorSet.pImageInfo	  = nullptr;
		// Default value in all examples
		writeDescriptorSet.descriptorCount = descriptorCount;
		return writeDescriptorSet;
	}

	inline vk::WriteDescriptorSet write_descriptor_set(vk::DescriptorSet dstSet,
													   vk::DescriptorType type,
													   uint32_t binding,
													   vk::DescriptorImageInfo* imageInfo,
													   uint32_t descriptorCount = 1)
	{
		vk::WriteDescriptorSet writeDescriptorSet;
		writeDescriptorSet.pNext		  = NULL;
		writeDescriptorSet.dstSet		  = dstSet;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.dstBinding	  = binding;
		writeDescriptorSet.pImageInfo	  = imageInfo;
		// Default value in all examples
		writeDescriptorSet.descriptorCount = descriptorCount;
		writeDescriptorSet.pBufferInfo	   = nullptr;
		return writeDescriptorSet;
	}

	inline VkVertexInputBindingDescription
	vertex_input_binding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
	{
		VkVertexInputBindingDescription vInputBindDescription = {};
		vInputBindDescription.binding						  = binding;
		vInputBindDescription.stride						  = stride;
		vInputBindDescription.inputRate						  = inputRate;
		return vInputBindDescription;
	}

	inline VkVertexInputAttributeDescription
	vertex_input_attr(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset)
	{
		VkVertexInputAttributeDescription vInputAttribDescription = {};
		vInputAttribDescription.location						  = location;
		vInputAttribDescription.binding							  = binding;
		vInputAttribDescription.format							  = format;
		vInputAttribDescription.offset							  = offset;
		return vInputAttribDescription;
	}

	inline VkPipelineVertexInputStateCreateInfo pipeline_vertex_inputstate_ci()
	{
		VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
		pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		pipelineVertexInputStateCreateInfo.pNext = NULL;
		return pipelineVertexInputStateCreateInfo;
	}

	inline VkPipelineInputAssemblyStateCreateInfo
	pipeline_input_asmstate_ci(VkPrimitiveTopology topology,
							   VkPipelineInputAssemblyStateCreateFlags flags,
							   VkBool32 primitiveRestartEnable)
	{
		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {};
		pipelineInputAssemblyStateCreateInfo.sType	  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pipelineInputAssemblyStateCreateInfo.topology = topology;
		pipelineInputAssemblyStateCreateInfo.flags	  = flags;
		pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
		return pipelineInputAssemblyStateCreateInfo;
	}

	inline VkPipelineRasterizationStateCreateInfo
	pipeline_rasterizationstate_ci(VkPolygonMode polygonMode,
								   VkCullModeFlags cullMode,
								   VkFrontFace frontFace,
								   VkPipelineRasterizationStateCreateFlags flags)
	{
		VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {};
		pipelineRasterizationStateCreateInfo.sType		 = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
		pipelineRasterizationStateCreateInfo.cullMode	 = cullMode;
		pipelineRasterizationStateCreateInfo.frontFace	 = frontFace;
		pipelineRasterizationStateCreateInfo.flags		 = flags;
		pipelineRasterizationStateCreateInfo.depthClampEnable = VK_TRUE;
		pipelineRasterizationStateCreateInfo.lineWidth		  = 1.0f;
		return pipelineRasterizationStateCreateInfo;
	}

	inline VkPipelineColorBlendAttachmentState pipeline_colorblend_attch_state(VkColorComponentFlags colorWriteMask,
																			   VkBool32 blendEnable)
	{
		VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
		pipelineColorBlendAttachmentState.colorWriteMask					  = colorWriteMask;
		pipelineColorBlendAttachmentState.blendEnable						  = blendEnable;
		return pipelineColorBlendAttachmentState;
	}

	inline VkPipelineColorBlendStateCreateInfo
	pipeline_colorblendstate_ci(uint32_t attachmentCount, const VkPipelineColorBlendAttachmentState* pAttachments)
	{
		VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {};
		pipelineColorBlendStateCreateInfo.sType			  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		pipelineColorBlendStateCreateInfo.pNext			  = NULL;
		pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
		pipelineColorBlendStateCreateInfo.pAttachments	  = pAttachments;
		return pipelineColorBlendStateCreateInfo;
	}

	inline VkPipelineDepthStencilStateCreateInfo
	pipeline_ds_state_ci(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp)
	{
		VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {};
		pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pipelineDepthStencilStateCreateInfo.depthTestEnable	 = depthTestEnable;
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
		pipelineDepthStencilStateCreateInfo.depthCompareOp	 = depthCompareOp;
		pipelineDepthStencilStateCreateInfo.front			 = pipelineDepthStencilStateCreateInfo.back;
		pipelineDepthStencilStateCreateInfo.back.compareOp	 = VK_COMPARE_OP_ALWAYS;
		return pipelineDepthStencilStateCreateInfo;
	}

	inline VkPipelineViewportStateCreateInfo
	pipeline_viewportstate_ci(uint32_t viewportCount, uint32_t scissorCount, VkPipelineViewportStateCreateFlags flags)
	{
		VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {};
		pipelineViewportStateCreateInfo.sType		  = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		pipelineViewportStateCreateInfo.viewportCount = viewportCount;
		pipelineViewportStateCreateInfo.scissorCount  = scissorCount;
		pipelineViewportStateCreateInfo.flags		  = flags;
		return pipelineViewportStateCreateInfo;
	}

	inline VkPipelineMultisampleStateCreateInfo pipeline_ms_state_ci(VkSampleCountFlagBits rasterizationSamples,
																	 VkPipelineMultisampleStateCreateFlags flags)
	{
		VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
		pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
		return pipelineMultisampleStateCreateInfo;
	}

	inline VkPipelineDynamicStateCreateInfo pipeline_dynstate_ci(const VkDynamicState* pDynamicStates,
																 uint32_t dynamicStateCount,
																 VkPipelineDynamicStateCreateFlags flags)
	{
		VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {};
		pipelineDynamicStateCreateInfo.sType			 = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		pipelineDynamicStateCreateInfo.pDynamicStates	 = pDynamicStates;
		pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
		return pipelineDynamicStateCreateInfo;
	}

	inline VkPipelineTessellationStateCreateInfo pipeline_tess_state_ci(uint32_t patchControlPoints)
	{
		VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo = {};
		pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
		return pipelineTessellationStateCreateInfo;
	}

	inline VkGraphicsPipelineCreateInfo
	pipeline_ci(VkPipelineLayout layout, VkRenderPass renderPass, VkPipelineCreateFlags flags)
	{
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType						= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.pNext						= NULL;
		pipelineCreateInfo.layout						= layout;
		pipelineCreateInfo.renderPass					= renderPass;
		pipelineCreateInfo.flags						= flags;
		return pipelineCreateInfo;
	}

	inline VkComputePipelineCreateInfo compute_pipeline_ci(VkPipelineLayout layout, VkPipelineCreateFlags flags)
	{
		VkComputePipelineCreateInfo computePipelineCreateInfo = {};
		computePipelineCreateInfo.sType						  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.layout					  = layout;
		computePipelineCreateInfo.flags						  = flags;
		return computePipelineCreateInfo;
	}

	inline VkPushConstantRange push_constant_range(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset)
	{
		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags		  = stageFlags;
		pushConstantRange.offset			  = offset;
		pushConstantRange.size				  = size;
		return pushConstantRange;
	}
}	 // namespace utility::vulkan::defaults

namespace utility::vulkan
{
	inline bool check(const vk::Result& value)
	{
		psl_assert(value == vk::Result::eSuccess, "vk::Result expected success, but got {}", vk::to_string(value));
		return value == vk::Result::eSuccess;
	}

	inline bool check(const VkResult& value)
	{
		psl_assert(value == VkResult::VK_SUCCESS, "vk::Result expected success, but got {}", vk::to_string((vk::Result)value));
		return value == VkResult::VK_SUCCESS;
	}

	template <typename T>
	inline bool check(const vk::ResultValue<T>& value)
	{
		return check(value.result);
	}

	inline vk::CommandBuffer create_cmd_buffer(vk::Device device,
											   vk::CommandPool pool,
											   vk::CommandBufferLevel level,
											   bool begin,
											   uint32_t bufferCount)
	{
		vk::CommandBuffer cmdBuffer;

		vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.setCommandPool(pool);
		commandBufferAllocateInfo.setLevel(level);
		commandBufferAllocateInfo.setCommandBufferCount(bufferCount);

		check(device.allocateCommandBuffers(&commandBufferAllocateInfo, &cmdBuffer));

		// If requested, also start the new command buffer
		if(begin)
		{
			vk::CommandBufferBeginInfo cmdBufferBeginInfo;
			check(cmdBuffer.begin(&cmdBufferBeginInfo));
		}

		return cmdBuffer;
	}

	inline vk::Bool32 supported_depthformat(const vk::PhysicalDevice& physicalDevice,
											std::vector<vk::Format> depthFormats,
											vk::Format* depthFormat)
	{
		for(auto format : depthFormats)
		{
			vk::FormatProperties formatProps;
			physicalDevice.getFormatProperties(format, &formatProps);
			// Format must support depth stencil attachment for optimal tiling
			if(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
			{
				*depthFormat = format;
				return true;
			}
		}

		return false;
	}

	inline vk::Bool32 supported_depthformat(const vk::PhysicalDevice& physicalDevice, vk::Format* depthFormat)
	{
		// Since all depth formats may be optional, we need to find a suitable depth format to use
		// Start with the highest precision packed format
		std::vector<vk::Format> depthFormats = {vk::Format::eD32SfloatS8Uint,
												vk::Format::eD32Sfloat,
												vk::Format::eD24UnormS8Uint,
												vk::Format::eD16UnormS8Uint,
												vk::Format::eD16Unorm};

		return supported_depthformat(physicalDevice, depthFormats, depthFormat);
	}

	// Create an image memory barrier for changing the layout of
	// an image and put it into an active command buffer
	// See chapter 11.4 "Image Layout" for details

	inline vk::ImageMemoryBarrier image_memory_barrier_for(vk::ImageLayout oldImageLayout,
														   vk::ImageLayout newImageLayout)
	{
		vk::ImageMemoryBarrier imageMemoryBarrier {};
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;

		// Source layouts (old)
		// Source access mask controls actions that have to be finished on the old layout
		// before it will be transitioned to the new layout
		switch(oldImageLayout)
		{
		case vk::ImageLayout::eGeneral:
		case vk::ImageLayout::eUndefined:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			imageMemoryBarrier.srcAccessMask = (vk::AccessFlagBits)(0);
			break;

		case vk::ImageLayout::ePreinitialized:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
			break;

		case vk::ImageLayout::eColorAttachmentOptimal:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
			break;

		case vk::ImageLayout::eDepthStencilAttachmentOptimal:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			break;

		case vk::ImageLayout::eTransferSrcOptimal:
			// Image is a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
			break;

		case vk::ImageLayout::eTransferDstOptimal:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			break;

		case vk::ImageLayout::eShaderReadOnlyOptimal:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
			break;
		default:
			throw std::runtime_error("unhandled format");
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch(newImageLayout)
		{
		case vk::ImageLayout::eTransferDstOptimal:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
			break;

		case vk::ImageLayout::eTransferSrcOptimal:
			// Image will be used as a transfer source
			// Make sure any reads from and writes to the image have been finished
			imageMemoryBarrier.srcAccessMask = imageMemoryBarrier.srcAccessMask | vk::AccessFlagBits::eTransferRead;
			imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
			break;

		case vk::ImageLayout::eColorAttachmentOptimal:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
			imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
			break;

		case vk::ImageLayout::eDepthStencilAttachmentOptimal:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			imageMemoryBarrier.dstAccessMask =
			  imageMemoryBarrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			break;

		case vk::ImageLayout::eShaderReadOnlyOptimal:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if(imageMemoryBarrier.srcAccessMask == (vk::AccessFlagBits)0)
			{
				imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
			}
			imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			break;
		default:
			throw std::runtime_error("unhandled format");
		}

		return imageMemoryBarrier;
	}

	inline void set_image_layout(vk::CommandBuffer cmdbuffer,
								 vk::Image image,
								 vk::ImageLayout oldImageLayout,
								 vk::ImageLayout newImageLayout,
								 vk::ImageSubresourceRange subresourceRange,
								 vk::PipelineStageFlags srcStageMask,
								 vk::PipelineStageFlags dstStageMask)
	{
		// Create an image barrier object
		vk::ImageMemoryBarrier imageMemoryBarrier = image_memory_barrier_for(oldImageLayout, newImageLayout);
		imageMemoryBarrier.setPNext(nullptr);
		imageMemoryBarrier.image			= image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		// Put barrier inside setup command buffer
		cmdbuffer.pipelineBarrier(
		  srcStageMask, dstStageMask, (vk::DependencyFlagBits)0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}

	inline void set_image_layout(vk::CommandBuffer& cmdbuffer,
								 vk::Image& image,
								 const vk::ImageLayout& oldImageLayout,
								 const vk::ImageLayout& newImageLayout,
								 vk::ImageSubresourceRange& subresourceRange)
	{
		set_image_layout(cmdbuffer,
						 image,
						 oldImageLayout,
						 newImageLayout,
						 subresourceRange,
						 vk::PipelineStageFlagBits::eAllCommands,
						 vk::PipelineStageFlagBits::eAllCommands);
		return;
	}


	inline vk::CommandBuffer CreateCommandBuffer(vk::Device device,
												 vk::CommandPool pool,
												 vk::CommandBufferLevel level,
												 bool begin,
												 uint32_t bufferCount)
	{
		vk::CommandBuffer cmdBuffer;

		vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.setCommandPool(pool);
		commandBufferAllocateInfo.setLevel(level);
		commandBufferAllocateInfo.setCommandBufferCount(bufferCount);

		check(device.allocateCommandBuffers(&commandBufferAllocateInfo, &cmdBuffer));

		// If requested, also start the new command buffer
		if(begin)
		{
			vk::CommandBufferBeginInfo cmdBufferBeginInfo;
			check(cmdBuffer.begin(&cmdBufferBeginInfo));
		}

		return cmdBuffer;
	}


	/**
	 * @brief Returns true if the attachment has a depth component
	 */
	inline bool has_depth(vk::Format format)
	{
		static std::vector<vk::Format> formats = {
		  vk::Format::eD16Unorm,
		  vk::Format::eX8D24UnormPack32,
		  vk::Format::eD32Sfloat,
		  vk::Format::eD16UnormS8Uint,
		  vk::Format::eD24UnormS8Uint,
		  vk::Format::eD32SfloatS8Uint,
		};
		return std::find(formats.begin(), formats.end(), format) != std::end(formats);
	}

	/**
	 * @brief Returns true if the attachment has a stencil component
	 */
	inline bool has_stencil(vk::Format format)
	{
		static std::vector<vk::Format> formats = {
		  vk::Format::eS8Uint,
		  vk::Format::eD16UnormS8Uint,
		  vk::Format::eD24UnormS8Uint,
		  vk::Format::eD32SfloatS8Uint,
		};
		return std::find(formats.begin(), formats.end(), format) != std::end(formats);
	}
	/**
	 * @brief Returns true if the attachment is a depth and/or stencil attachment
	 */
	inline bool is_depthstencil(vk::Format format) { return (has_depth(format) && has_stencil(format)); }
}	 // namespace utility::vulkan
